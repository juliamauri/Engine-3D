#ifndef __QUADTREE_H__
#define __QUADTREE_H__

#include "RE_GameObject.h"
#include "MathGeoLib\include\Geometry\AABB.h"

#include "PoolMapped.h"

#include <EASTL/list.h>
#include <EASTL/iterator.h>
#include <EASTL/stack.h>

class QTree
{
public:
	QTree();
	~QTree();

	void	Build(RE_GameObject* root_g_obj);
	void	BuildFromList(const AABB& box, const eastl::list<RE_GameObject*>& gos);
	void	Draw() const;

	void	SetDrawMode(short mode);
	short	GetDrawMode() const;

	void	Pop(const RE_GameObject* g_obj);

	template<typename TYPE>
	inline void CollectIntersections(eastl::vector<RE_GameObject*>& objects, const TYPE & primitive) const;

private:

	void Push(RE_GameObject* g_obj);
	void Clear();
	void PushWithChilds(RE_GameObject* g_obj);

private:

	class QTreeNode
	{
	public:
		QTreeNode();
		QTreeNode(const AABB& box, QTreeNode* parent = nullptr);
		~QTreeNode();

		void Push(RE_GameObject* g_obj);
		void Pop(const RE_GameObject* g_obj);
		void Clear();

		void Draw(const int* edges, int count) const;

		void SetBox(const AABB& bounding_box);
		const AABB& GetBox() const;

		template<typename TYPE>
		inline void CollectIntersections(eastl::vector<RE_GameObject*>& objects, const TYPE & primitive) const;

	private:

		void AddNodes();
		void Distribute();

	private:

		QTreeNode* nodes[4];
		QTreeNode* parent = nullptr;

		eastl::list<RE_GameObject*> g_objs;
		bool is_leaf = true;
		AABB box;
	} root;

	enum DrawMode : short
	{
		DISABLED,
		TOP,
		BOTTOM,
		TOP_BOTTOM,
		ALL
	} draw_mode = DISABLED;

	int edges[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	int count = 0;
};

template<typename TYPE>
inline void QTree::QTreeNode::CollectIntersections(eastl::vector<RE_GameObject*>& objects, const TYPE & primitive) const
{
	if (primitive.Intersects(box))
	{
		for (auto go : g_objs)
			if (primitive.Intersects(go->GetGlobalBoundingBox()))
				objects.push_back(go);

		if (!is_leaf)
			for (int i = 0; i < 4; ++i)
				if (nodes[i] != nullptr) nodes[i]->CollectIntersections(objects, primitive);
	}
}

template<typename TYPE>
inline void QTree::CollectIntersections(eastl::vector<RE_GameObject*>& objects, const TYPE & primitive) const
{
	root.CollectIntersections(objects, primitive);
}

struct AABBDynamicTreeNode
{
	AABB box;
	UID object_index = 0;
	int parent_index = -1, child1 = -1, child2 = -1;
	bool is_leaf = true;
};

class AABBDynamicTree : public PoolMapped<AABBDynamicTreeNode,int, 1024, 512>
{
private:

	int randomCount = 0, size = 0, node_count = 0, root_index = -1;

public:

	AABBDynamicTree();
	~AABBDynamicTree();

	void PushNode(UID goUID, AABB box);
	void PopNode(UID index);
	eastl::vector<int> GetAllKeys() const override;
	void Clear();
	void CollectIntersections(Ray ray, eastl::stack<UID>& indexes) const;
	void CollectIntersections(const Frustum frustum, eastl::stack<UID>& indexes) const;

	void Draw()const;
	int GetCount() const;

	eastl::map<UID, int> objectToNode;

private:

	void Rotate(AABBDynamicTreeNode& node, int index);

	int AllocateLeafNode(AABB box, UID index);
	int AllocateInternalNode();

	static inline AABB Union(AABB box1, AABB box2);
	static inline void SetLeaf(AABBDynamicTreeNode& node, AABB box, UID index);
	static inline void SetInternal(AABBDynamicTreeNode& node);
};

#endif // !__QUADTREE_H__