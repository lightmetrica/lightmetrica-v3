#include <lm/lm.h>
#pragma warning(push)
#pragma warning(disable:4244)
#undef _CRT_SECURE_NO_WARNINGS
#include <pbrtParser/Scene.h>
#pragma warning(pop)

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// ----------------------------------------------------------------------------

namespace {

Vec3 pbrtVecToLmVec3(const pbrt::vec3f& v) {
	return Vec3(v.x, v.y, v.z);
}

Vec2 pbrtVecToLmVec2(const pbrt::vec2f& v) {
	return Vec2(v.x, v.y);
}

}

// ----------------------------------------------------------------------------

class Mesh_PBRT : public Mesh {
private:
	pbrt::TriangleMesh* pbrtMesh_;

public:
	virtual bool construct(const Json& prop) override {
		pbrtMesh_ = prop["mesh_"].get<pbrt::TriangleMesh*>();
		assert(pbrtMesh_);
		return true;
	}

	virtual void foreachTriangle(const ProcessTriangleFunc& processTriangle) const override {
		for (int fi = 0; fi < int(pbrtMesh_->index.size()); fi++) {
			processTriangle(fi, triangleAt(fi));
		}
	}

	virtual Tri triangleAt(int face) const override {
		const auto pointByFace = [&](int fi) -> Point {
			return Point{
				pbrtVecToLmVec3(pbrtMesh_->vertex[fi]),
				pbrtMesh_->normal.empty()
					? Vec3()
					: pbrtVecToLmVec3(pbrtMesh_->normal[fi]),
				pbrtMesh_->texcoord.empty()
					? Vec3()
					: pbrtVecToLmVec2(pbrtMesh_->texcoord[fi])
			};
		};
		const auto index = pbrtMesh_->index[face];
		return Tri{
			pointByFace(index.x),
			pointByFace(index.y),
			pointByFace(index.z)
		};
	}

	virtual Point surfacePoint(int face, Vec2 uv) const override {
		const auto index = pbrtMesh_->index[face];
		const auto p1 = pbrtVecToLmVec3(pbrtMesh_->vertex[index.x]);
		const auto p2 = pbrtVecToLmVec3(pbrtMesh_->vertex[index.y]);
		const auto p3 = pbrtVecToLmVec3(pbrtMesh_->vertex[index.z]);
		Point p;
		// Position
		p.p = math::mixBarycentric(p1, p2, p3, uv);
		// Normal
		p.n = pbrtMesh_->normal.empty()
			? glm::normalize(glm::cross(p2 - p1, p3 - p1))
			: glm::normalize(math::mixBarycentric(
				pbrtVecToLmVec3(pbrtMesh_->normal[index.x]),
				pbrtVecToLmVec3(pbrtMesh_->normal[index.y]),
				pbrtVecToLmVec3(pbrtMesh_->normal[index.z]), uv));
		// Texture coordinates
		p.t = pbrtMesh_->texcoord.empty()
			? Vec3()
			: math::mixBarycentric(
				pbrtVecToLmVec2(pbrtMesh_->texcoord[index.x]),
				pbrtVecToLmVec2(pbrtMesh_->texcoord[index.y]),
				pbrtVecToLmVec2(pbrtMesh_->texcoord[index.z]), uv);
		return p;
	}

	virtual int numTriangles() const override {
		return int(pbrtMesh_->index.size());
	}
};

LM_COMP_REG_IMPL(Mesh_PBRT, "mesh::pbrt");

// ----------------------------------------------------------------------------

class Model_PBRT : public Model {
private:
	pbrt::Scene::SP pbrtScene_;
	std::vector<Ptr<Mesh>> meshes_;
	Ptr<Material> defaultMaterial_;

public:
	virtual bool construct(const Json& prop) override {
		// Load PBRT scene
		const std::string path = prop["path"];
		pbrtScene_ = pbrt::importPBRT(path);
		if (!pbrtScene_) {
			return false;
		}

		// Load meshes
		int cnt = 0;
		for (auto shape : pbrtScene_->world->shapes) {
			auto mesh = std::dynamic_pointer_cast<pbrt::TriangleMesh>(shape);
			if (!mesh) {
				continue;
			}
			auto lmMesh = lm::comp::create<Mesh>("mesh::pbrt", makeLoc(std::to_string(cnt++)), {
				{"mesh_", mesh.get()}
			});
			if (!lmMesh) {
				LM_ERROR("Failed to convert mesh");
				return false;
			}
			meshes_.push_back(std::move(lmMesh));
		}

		// Create default material
		defaultMaterial_ = lm::comp::create<Material>("material::diffuse", makeLoc("defautMaterial"), {
			{"Kd", Vec3(1,1,1)}
		});

		return true;
	}

	virtual void createPrimitives(const CreatePrimitiveFunc& createPrimitive) const override {
		for (const auto& mesh : meshes_) {
			createPrimitive(mesh.get(), defaultMaterial_.get(), nullptr);
		}
	}
};

LM_COMP_REG_IMPL(Model_PBRT, "model::pbrt");

LM_NAMESPACE_END(LM_NAMESPACE)
