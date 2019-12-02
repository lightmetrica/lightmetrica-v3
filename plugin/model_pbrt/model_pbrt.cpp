#include <lm/lm.h>
#pragma warning(push)
#pragma warning(disable:4244)
#undef _CRT_SECURE_NO_WARNINGS
#include <pbrtParser/Scene.h>
#pragma warning(pop)

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// ------------------------------------------------------------------------------------------------

namespace {

Vec3 convertPbrtVec3(const pbrt::vec3f& v) {
	return Vec3(v.x, v.y, v.z);
}

Vec2 convertPbrtVec2(const pbrt::vec2f& v) {
	return Vec2(v.x, v.y);
}

Mat3 convertPbrtMat3(const pbrt::math::mat3f& m) {
	return Mat3(
		convertPbrtVec3(m.vx),
		convertPbrtVec3(m.vy),
		convertPbrtVec3(m.vz));
}

Mat4 convertPbrtXfm(const pbrt::affine3f& v) {
	const auto rotM = Mat4(convertPbrtMat3(v.l));
	const auto transM = glm::translate(convertPbrtVec3(v.p));
	return transM * rotM;
}

}

// ------------------------------------------------------------------------------------------------

class Mesh_PBRT : public Mesh {
private:
	pbrt::TriangleMesh* pbrtMesh_;

public:
	virtual void construct(const Json& prop) override {
		pbrtMesh_ = prop["mesh_"].get<pbrt::TriangleMesh*>();
		assert(pbrtMesh_);
	}

	virtual void foreach_triangle(const ProcessTriangleFunc& processTriangle) const override {
		for (int fi = 0; fi < int(pbrtMesh_->index.size()); fi++) {
			processTriangle(fi, triangle_at(fi));
		}
	}

	virtual Tri triangle_at(int face) const override {
		const auto pointByFace = [&](int fi) -> Point {
			return Point{
				convertPbrtVec3(pbrtMesh_->vertex[fi]),
				pbrtMesh_->normal.empty()
					? Vec3()
					: convertPbrtVec3(pbrtMesh_->normal[fi]),
				pbrtMesh_->texcoord.empty()
					? Vec3()
					: convertPbrtVec2(pbrtMesh_->texcoord[fi])
			};
		};
		const auto index = pbrtMesh_->index[face];
		return Tri{
			pointByFace(index.x),
			pointByFace(index.y),
			pointByFace(index.z)
		};
	}

	virtual Point surface_point(int face, Vec2 uv) const override {
		const auto index = pbrtMesh_->index[face];
		const auto p1 = convertPbrtVec3(pbrtMesh_->vertex[index.x]);
		const auto p2 = convertPbrtVec3(pbrtMesh_->vertex[index.y]);
		const auto p3 = convertPbrtVec3(pbrtMesh_->vertex[index.z]);
		Point p;
		// Position
		p.p = math::mix_barycentric(p1, p2, p3, uv);
		// Normal
		p.n = pbrtMesh_->normal.empty()
			? glm::normalize(glm::cross(p2 - p1, p3 - p1))
			: glm::normalize(math::mix_barycentric(
				convertPbrtVec3(pbrtMesh_->normal[index.x]),
				convertPbrtVec3(pbrtMesh_->normal[index.y]),
				convertPbrtVec3(pbrtMesh_->normal[index.z]), uv));
		// Texture coordinates
		p.t = pbrtMesh_->texcoord.empty()
			? Vec3()
			: math::mix_barycentric(
				convertPbrtVec2(pbrtMesh_->texcoord[index.x]),
				convertPbrtVec2(pbrtMesh_->texcoord[index.y]),
				convertPbrtVec2(pbrtMesh_->texcoord[index.z]), uv);
		return p;
	}

	virtual int num_triangles() const override {
		return int(pbrtMesh_->index.size());
	}
};

LM_COMP_REG_IMPL(Mesh_PBRT, "mesh::pbrt");

// ------------------------------------------------------------------------------------------------

class Model_PBRT : public Model {
private:
	pbrt::Scene::SP pbrtScene_;			// Scene of PBRT parser
	std::vector<Ptr<Mesh>> meshes_;		// Underlying meshes
	Ptr<Camera> camera_;				// Underlying camera
	Ptr<Material> defaultMaterial_;		// Default material (TODO: replace after material support)
	std::vector<SceneNode> nodes_;		// Scene nodes
	
public:
	Model_PBRT() {
		// Index 0 is fixed to the scene group
		nodes_.push_back(SceneNode::make_group(0, false, {}));
	}

public:
	virtual void construct(const Json& prop) override {
		// Load PBRT scene
		const std::string path = json::value<std::string>(prop, "path");
		pbrtScene_ = pbrt::importPBRT(path);
		if (!pbrtScene_) {
            LM_THROW_EXCEPTION(Error::IOError, "Failed to load PBRT scene [path='{}']", path);
		}

		// Load camera
		if (!pbrtScene_->cameras.empty()) {
			// View matrix
			const auto pbrtCamera = pbrtScene_->cameras[0];
			auto viewM = convertPbrtXfm(pbrtCamera->frame);

			// Create camera asset
			camera_ = lm::comp::create<Camera>("camera::pinhole", make_loc("camera"), {
				{"matrix", viewM},
				{"vfov", Float(pbrtCamera->fov)}
			});
			if (!camera_) {
                LM_THROW_EXCEPTION(Error::InvalidArgument, "Failed to create camera");
			}

			// Add camera primitive node
			const int index = int(nodes_.size());
			nodes_.push_back(SceneNode::make_primitive(
				index,
				nullptr,
				nullptr,
				nullptr,
				camera_.get(),
				nullptr
			));
			nodes_[0].group.children.push_back(index);
		}

		// Create default material
		defaultMaterial_ = lm::comp::create<Material>("material::diffuse", make_loc("defautMaterial"), {
			{"Kd", Vec3(1,1,1)}
		});

		// Load meshes
		int meshCount = 0;
		std::unordered_map<std::string, int> visited;	// Name of PBRT object -> node index
		using VisitObjectFunc = std::function<void(int, pbrt::Object::SP, const pbrt::affine3f&)>;
		const VisitObjectFunc visitObject = [&](int parent, pbrt::Object::SP pbrtObject, const pbrt::affine3f& instanceXfm) {
			// Process the underlying shapes
			if (!pbrtObject->shapes.empty()) {
				// For each shape in the pbrt object
				for (auto& shape : pbrtObject->shapes) {
					// Create a mesh
					auto mesh = std::dynamic_pointer_cast<pbrt::TriangleMesh>(shape);
					if (!mesh) {
						continue;
					}
					auto lmMesh = lm::comp::create<Mesh>("mesh::pbrt", make_loc(std::to_string(meshCount++)), {
						{"mesh_", mesh.get()}
					});
					assert(lmMesh);
					meshes_.push_back(std::move(lmMesh));

					// Create a scene node
					const int index = int(nodes_.size());
					nodes_.push_back(SceneNode::make_primitive(
						index,
						meshes_.back().get(),
						defaultMaterial_.get(),
						nullptr,
						nullptr,
						nullptr
					));
					nodes_[parent].group.children.push_back(index);
				}
			}

			// Process the instanced objects
			if (!pbrtObject->instances.empty()) {
				for (auto inst : pbrtObject->instances) {
					// Global transform
					const auto globalXfm = instanceXfm * inst->xfm;

					// Create a transform group
					const int transformGroupIndex = int(nodes_.size());
					nodes_.push_back(SceneNode::make_group(
						transformGroupIndex,
						false,
						convertPbrtXfm(globalXfm)
					));
					auto& transformGroup = nodes_.back();
					nodes_[parent].group.children.push_back(transformGroupIndex);

					// If we already created the instance group, use it
					const auto it = visited.find(inst->object->name);
					if (it != visited.end()) {
						transformGroup.group.children.push_back(it->second);
					}
					// Otherwise, recursively process the child node
					else {
						// Create instance group node
						const int instanceGroupNodeIndex = int(nodes_.size());
						nodes_.push_back(SceneNode::make_group(
							instanceGroupNodeIndex,
							true,
							Mat4(1_f)
						));
						visitObject(instanceGroupNodeIndex, inst->object, globalXfm);
						visited[inst->object->name] = instanceGroupNodeIndex;
					}
				}
			}
		};
		visitObject(0, pbrtScene_->world, pbrt::affine3f::identity());
	}

	virtual void create_primitives(const CreatePrimitiveFunc& createPrimitive) const override {
		for (const auto& mesh : meshes_) {
			createPrimitive(mesh.get(), defaultMaterial_.get(), nullptr);
		}
	}

	virtual void foreach_node(const VisitNodeFuncType& visit) const override {
		for (const auto& node : nodes_) {
			visit(node);
		}
	}
};

LM_COMP_REG_IMPL(Model_PBRT, "model::pbrt");

LM_NAMESPACE_END(LM_NAMESPACE)
