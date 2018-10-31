/*
    Lightmetrica - Copyright (c) 2018 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#include <pch.h>
#include <lm/accel.h>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

struct Tri {
    Vec3 p1;       // One vertex of the triangle
    Vec3 e1, e2;   // Two edges indident to p1
    Vec3 n;        // Normal
    Bound b;       // Bound of the triangle
    Vec3 c;        // Center of the bound
    int oi;        // Object index
    int fi;        // Face index

    Tri(Vec3 p1, Vec3 p2, Vec3 p3, int oi, int fi)
        : p1(p1), oi(oi), fi(fi) {
        e1 = p2 - p1;
        e2 = p3 - p1;
        n = glm::normalize(glm::cross(p2 - p1, p3 - p1));
        b = merge(b, p1);
        b = merge(b, p2);
        b = merge(b, p3);
        c = b.center();
    }

    // Checks intersection with a ray [Möller & Trumbore 1997]
    struct Hit {
        Float t;     // Distance to the triangle
        Float u, v;  // Hitpoint in barycentric coodinates
    };
    std::optional<Hit> isect(Ray r, Float tl, Float th) const {
        auto p = cross(r.d, e2);
        auto tv = r.o - p1;
        auto q = cross(tv, e1);
        auto d = dot(e1, p);
        auto ad = abs(d);
        auto s = copysign(1_f, d);
        auto u = dot(tv, p) * s;
        auto v = dot(r.d, q) * s;
        if (ad < 1e-8_f || u < 0_f || v < 0_f || u + v > ad) {
            return {};
        }
        auto t = dot(e2, q) / d;
        if (t < tl || th < t) {
            return {};
        }
        return Hit{ t, u / ad, v / ad };
    }
};

class Accel_SAHBVH : public Accel {
private:
    // BVH node
    struct Node {
        Bound b;        // Bound of the node
        bool leaf = 0;  // True if the node is leaf
        int s, e;       // Range of triangle indices (valid only in leaf nodes)
        int c1, c2;     // Index to the child nodes
    };
    std::vector<Node> ns;  // Nodes
    std::vector<Tri> trs;  // Triangles
    std::vector<int> ti;   // Triangle indices

public:
    virtual void build(const Scene& scene) const override {
        // Setup triangle list
        scene.forachUnderlying([](Component* p_) {
            const auto* p = p_->cast<Primitive>();
            
        });


        trs = trs_;
        const int nt = int(trs.size()); // Number of triangles
        struct Entry {
            int index;
            int start;
            int end;
        };
        std::queue<Entry> q;            // Queue for traversal (node index, start, end)
        q.push({0, 0, nt});             // Initialize the queue with root node
        ns.assign(2 * nt - 1, {});      // Maximum number of nodes: 2*nt-1
        ti.assign(nt, 0);
        std::iota(ti.begin(), ti.end(), 0);
        std::mutex mu;                  // For concurrent queue
        std::condition_variable cv;     // For concurrent queue
        std::atomic<int> pr = 0;        // Processed triangles
        std::atomic<int> nn = 1;        // Number of current nodes
        bool done = 0;                  // True if the build process is done

        LM_INFO("Building acceleration structure");
        auto process = [&]() {
            while (!done) {
                // Each step construct a node for the triangles ranges in [s,e)
                auto [ni, s, e] = [&]() -> std::tuple<int, int, int> {
                    std::unique_lock<std::mutex> lk(mu);
                    if (q.empty()) {
                        cv.wait(lk, [&]() { return done || !q.empty(); });
                    }
                    if (done)
                        return {};
                    auto v = q.front();
                    q.pop();
                    return v;
                }();
                if (done) {
                    break;
                }
                // Calculate the bound for the node
                Node& n = ns[ni];
                for (int i = s; i < e; i++) {
                    n.b = merge(n.b, trs[ti[i]].b);
                }
                // Function to sort the triangles according to the given axis
                auto st = [&, s = s, e = e](int ax) {
                    auto cmp = [&](int i1, int i2) {
                        return trs[i1].c[ax] < trs[i2].c[ax];
                    };
                    std::sort(&ti[s], &ti[e - 1] + 1, cmp);
                };
                // Function to create a leaf node
                auto lf = [&, s = s, e = e]() {
                    n.leaf = 1;
                    n.s = s;
                    n.e = e;
                    pr += e - s;
                    if (pr == int(trs.size())) {
                        done = 1;
                        cv.notify_all();
                    }
                };
                // Create a leaf node if the number of triangle is 1
                if (e - s < 2) {
                    lf();
                    continue;
                }
                // Selects a split axis and position according to SAH
                F b = Inf;
                int bi, ba;
                for (int a = 0; a < 3; a++) {
                    thread_local std::vector<F> l(nt + 1), r(nt + 1);
                    st(a);
                    B bl, br;
                    for (int i = 0; i <= e - s; i++) {
                        int j = e - s - i;
                        l[i] = bl.sa() * i;
                        r[j] = br.sa() * i;
                        bl = i < e - s ? merge(bl, trs[ti[s + i]].b) : bl;
                        br = j > 0 ? merge(br, trs[ti[s + j - 1]].b) : br;
                    }
                    for (int i = 1; i < e - s; i++) {
                        F c = 1 + (l[i] + r[i]) / n.b.sa();
                        if (c < b) {
                            b = c;
                            bi = i;
                            ba = a;
                        }
                    }
                }
                if (b > e - s) {
                    lf();
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
        std::vector<std::thread> ths(omp_get_max_threads());
        for (auto& th : ths) {
            th = std::thread(process);
        }
        for (auto& th : ths) {
            th.join();
        }
    };

    virtual std::optional<Hit> intersect(Ray ray, Float tmin, Float tmax) override {
        
    }
};

LM_COMP_REG_IMPL(Accel_SAHBVH, "accel::sahbvh");

LM_NAMESPACE_END(LM_NAMESPACE)
