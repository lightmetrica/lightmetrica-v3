/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/accel.h>
#include <lm/scene.h>
#include <lm/mesh.h>
#include <lm/logger.h>
#include <lm/exception.h>
#include <lm/serial.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

struct Tri {
    Vec3 p1;        // One vertex of the triangle
    Vec3 e1, e2;    // Two edges incident to p1
    Bound b;        // Bound of the triangle
    Vec3 c;         // Center of the bound
    int group;      // Group index
    int primitive;  // Primitive index associated to the triangle
    int face;       // Face index of the mesh associated to the triangle

    template <typename Archive>
    void serialize(Archive& ar) {
        ar(p1, e1, e2, b, c, group, primitive, face);
    }

    Tri() {}

    Tri(Vec3 p1, Vec3 p2, Vec3 p3, int group, int primitive, int face)
        : p1(p1), group(group), primitive(primitive), face(face) {
        e1 = p2 - p1;
        e2 = p3 - p1;
        b = merge(b, p1);
        b = merge(b, p2);
        b = merge(b, p3);
        c = b.center();
    }

    // Hit information
    struct Hit {
        Float t;     // Distance to the triangle
        Float u, v;  // Hitpoint in barycentric coordinates
    };

    // Checks intersection with a ray [Möller & Trumbore 1997]
    std::optional<Hit> isect(Ray r, Float tl, Float th) const {
        auto p = glm::cross(r.d, e2);
        auto tv = r.o - p1;
        auto q = glm::cross(tv, e1);
        auto d = glm::dot(e1, p);
        auto ad = glm::abs(d);
        auto s = std::copysign(1_f, d);
        auto u = glm::dot(tv, p) * s;
        auto v = glm::dot(r.d, q) * s;
        if (ad < 1e-8_f || u < 0_f || v < 0_f || u + v > ad) {
            return {};
        }
        auto t = glm::dot(e2, q) / d;
        if (t < tl || th < t) {
            return {};
        }
        return Hit{ t, u / ad, v / ad };
    }
};

// BVH node
struct Node {
    Bound b;        // Bound of the node
    bool leaf = 0;  // True if the node is leaf
    int s, e;       // Range of triangle indices (valid only in leaf nodes)
    int c1, c2;     // Index to the child nodes

    template <typename Archive>
    void serialize(Archive& ar) {
        ar(b, leaf, s, e, c1, c2);
    }
};

/*
\rst
.. function:: accel::sahbvh

   Bounding volume hierarchy with surface area heuristics.
   
   Features

   - Parallel construction.
   - Split axis is determined by longest axis.
   - Split position is determined by minimum SAH cost.
   - Uses full-sort of underlying geometries.
   - Uses triangle intersection by Möller and Trumbore [Möller1997]_.

   .. [Möller1997] T. Möller & B. Trumbore.
                   Fast, Minimum Storage Ray-Triangle Intersection.
                   Journal of Graphics Tools. 2(1):21--28. 1997.
\endrst
*/
class Accel_SAHBVH final : public Accel {
private:
    std::vector<Node> nodes_;   // Nodes
    std::vector<Tri> trs_;      // Triangles
    std::vector<int> indices_;  // Triangle indices
    
public:
    LM_SERIALIZE_IMPL(ar) {
        ar(nodes_, trs_, indices_);
    }

public:
    virtual void build(const Scene& scene) override {
        // Setup triangle list
        trs_.clear();
        scene.foreachTriangle([&](int group, int primitive, int face, Vec3 p1, Vec3 p2, Vec3 p3) {
            trs_.emplace_back(p1, p2, p3, group, primitive, face);
        });
        const int nt = int(trs_.size()); // Number of triangles
        struct Entry {
            int index;
            int start;
            int end;
        };
        std::queue<Entry> q;            // Queue for traversal (node index, start, end)
        q.push({0, 0, nt});             // Initialize the queue with root node
        nodes_.assign(2*nt-1, {});      // Maximum number of nodes: 2*nt-1
        indices_.assign(nt, 0);
        std::iota(indices_.begin(), indices_.end(), 0);
        std::mutex mu;                  // For concurrent queue
        std::condition_variable cv;     // For concurrent queue
        std::atomic<int> pr = 0;        // Processed triangles
        std::atomic<int> nn = 1;        // Number of current nodes
        bool done = 0;                  // True if the build process is done

        LM_INFO("Building acceleration structure [name='sahbvh']");
        auto process = [&]() {
            while (!done) {
                // Each step construct a node for the triangles ranges in [s,e)
                auto [ni, s, e] = [&]() -> Entry {
                    std::unique_lock<std::mutex> lk(mu);
                    if (!done && q.empty()) {
                        cv.wait(lk, [&]() { return done || !q.empty(); });
                    }
                    if (done) {
                        return {};
                    }
                    auto v = q.front();
                    q.pop();
                    return v;
                }();
                if (done) {
                    break;
                }

                // Calculate the bound for the node
                Node& n = nodes_[ni];
                for (int i = s; i < e; i++) {
                    n.b = merge(n.b, trs_[indices_[i]].b);
                }

                // Function to sort the triangles according to the given axis
                auto st = [&, s = s, e = e](int ax) {
                    auto cmp = [&](int i1, int i2) {
                        return trs_[i1].c[ax] < trs_[i2].c[ax];
                    };
                    std::sort(&indices_[s], &indices_[e-1]+1, cmp);
                };

                // Function to create a leaf node
                auto makeLeaf = [&, s = s, e = e]() {
                    n.leaf = 1;
                    n.s = s;
                    n.e = e;
                    pr += e - s;
                    if (pr == int(trs_.size())) {
                        std::unique_lock<std::mutex> lk(mu);
                        done = 1;
                        cv.notify_all();
                    }
                };

                // Create a leaf node if the number of triangle is 1
                if (e - s < 2) {
                    makeLeaf();
                    continue;
                }

                // Selects a split axis and position according to SAH
                Float b = Inf;
                int bi = -1, ba = -1;
                for (int a = 0; a < 3; a++) {
                    thread_local std::vector<Float> l(nt+1), r(nt+1);
                    st(a);
                    Bound bl, br;
                    for (int i = 0; i <= e - s; i++) {
                        int j = e - s - i;
                        l[i] = bl.surfaceArea() * i;
                        r[j] = br.surfaceArea() * i;
                        bl = i < e - s ? merge(bl, trs_[indices_[s+i]].b) : bl;
                        br = j > 0 ? merge(br, trs_[indices_[s+j-1]].b) : br;
                    }
                    for (int i = 1; i < e - s; i++) {
                        const auto c = 1_f + (l[i]+r[i])/n.b.surfaceArea();
                        if (c < b) {
                            b = c;
                            bi = i;
                            ba = a;
                        }
                    }
                }
                if (b > e - s) {
                    makeLeaf();
                    continue;
                }
                st(ba);
                int m = s + bi;
                std::unique_lock<std::mutex> lk(mu);
                q.push({n.c1 = nn++, s, m});
                q.push({n.c2 = nn++, m, e});
                cv.notify_one();
            }
        };
        std::vector<std::thread> ths(std::thread::hardware_concurrency());
        for (auto& th : ths) {
            th = std::thread(process);
        }
        for (auto& th : ths) {
            th.join();
        }
    };

    virtual std::optional<Hit> intersect(Ray ray, Float tmin, Float tmax) const override {
        exception::ScopedDisableFPEx guard_;  // Disable floating point exceptions
        std::optional<Tri::Hit> mh, h;
        int mi = -1;
        int s[99]{};
        int si = 0;
        while (si >= 0) {
            auto& n = nodes_.at(s[si--]);
            if (!n.b.isect(ray, tmin, tmax)) {
                continue;
            }
            if (!n.leaf) {
                s[++si] = n.c1;
                s[++si] = n.c2;
                continue;
            }
            for (int i = n.s; i < n.e; i++) {
                if (h = trs_[indices_[i]].isect(ray, tmin, tmax)) {
                    mh = h;
                    tmax = h->t;
                    mi = i;
                }
            }
        }
        if (!mh) {
            return {};
        }
        const auto& tr = trs_[indices_[mi]];
        return Hit{ tmax, Vec2(mh->u, mh->v), tr.group, tr.primitive, tr.face };
    }
};

LM_COMP_REG_IMPL(Accel_SAHBVH, "accel::sahbvh");

LM_NAMESPACE_END(LM_NAMESPACE)
