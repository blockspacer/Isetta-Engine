/*
 * Copyright (c) 2018 Isetta
 */
#include "BVTree.h"

#include <queue>
#include <unordered_set>
#include "Core/DataStructures/Array.h"
#include "Core/Debug/Assert.h"
#include "Core/Debug/DebugDraw.h"
#include "Ray.h"
#include "Scene/Entity.h"
#include "Util.h"
#include "brofiler/ProfilerCore/Brofiler.h"

namespace Isetta {
BVTree::~BVTree() {
  std::queue<Node*> q;
  q.push(root);
  while (!q.empty()) {
    Node* cur = q.front();
    q.pop();
    if (cur != nullptr) {
      q.push(cur->left);
      q.push(cur->right);
      delete cur;
    }
  }
}

void BVTree::AddCollider(Collider* const collider) {
  Node* newNode = MemoryManager::NewOnFreeList<Node>(collider);
  colNodeMap.insert({collider, newNode});
  AddNode(newNode);
}

void BVTree::RemoveCollider(Collider* const collider) {
  auto it = colNodeMap.find(collider);
  ASSERT(it != colNodeMap.end());
  RemoveNode(it->second, true);
  colNodeMap.erase(it);
}

void BVTree::Update() {
  PROFILE
  Array<Node*> toReInsert;

  std::queue<Node*> q;
  if (root != nullptr) {
    q.push(root);
  }

  while (!q.empty()) {
    Node* cur = q.front();
    q.pop();

    if (cur->left != nullptr) q.push(cur->left);
    if (cur->right != nullptr) q.push(cur->right);

    if (cur->IsLeaf() && !cur->IsInFatAABB()) {
      toReInsert.PushBack(cur);
    }
  }

  for (auto node : toReInsert) {
    RemoveNode(node, false);
  }

  for (auto node : toReInsert) {
    node->UpdateLeafAABB();
    AddNode(node);
  }

#if _EDITOR
  DebugDraw();
#endif
}

void BVTree::AddNode(Node* const newNode) {
  PROFILE
  AABB newAABB = newNode->aabb;

  if (root == nullptr) {
    root = newNode;
    root->parent = nullptr;
  } else {
    Node* cur = root;

    while (!cur->IsLeaf()) {
      float leftIncrease =
          AABB::Encapsulate(cur->left->aabb, newAABB).SurfaceArea() -
          cur->left->aabb.SurfaceArea();

      float rightIncrease =
          AABB::Encapsulate(cur->right->aabb, newAABB).SurfaceArea() -
          cur->right->aabb.SurfaceArea();

      if (leftIncrease > rightIncrease) {
        cur = cur->right;
      } else {
        cur = cur->left;
      }
    }

    if (cur == root) {
      // cur is root
      root = MemoryManager::NewOnFreeList<Node>(
          AABB::Encapsulate(cur->aabb, newAABB));
      cur->parent = root;
      newNode->parent = root;
      root->left = cur;
      root->right = newNode;
    } else {
      // cur is actual leaf, convert cur to branch
      Node* newBranch = MemoryManager::NewOnFreeList<Node>(
          AABB::Encapsulate(cur->aabb, newNode->aabb));
      newBranch->parent = cur->parent;
      cur->parent->SwapOutChild(cur, newBranch);
      cur->parent = newBranch;
      newNode->parent = newBranch;
      newBranch->left = cur;
      newBranch->right = newNode;

      Node* parent = newBranch->parent;

      while (parent != nullptr) {
        parent->UpdateBranchAABB();
        parent = parent->parent;
      }
    }
  }
}

void BVTree::RemoveNode(Node* const node, const bool deleteNode) {
  PROFILE
  ASSERT(node->IsLeaf());

  if (node == root) {
    root = nullptr;
  } else if (node->parent == root) {
    Node* newRoot;

    if (node == root->left) {
      newRoot = root->right;
    } else {
      newRoot = root->left;
    }

    MemoryManager::DeleteOnFreeList<Node>(root);
    root = newRoot;
    root->parent = nullptr;
  } else {
    Node* parent = node->parent;
    Node* grandParent = parent->parent;

    ASSERT(grandParent != nullptr);
    ASSERT(node == parent->left || node == parent->right);

    if (node == parent->left) {
      grandParent->SwapOutChild(parent, parent->right);
    } else {
      grandParent->SwapOutChild(parent, parent->left);
    }

    MemoryManager::DeleteOnFreeList<Node>(parent);

    Node* cur = grandParent;
    while (cur != nullptr) {
      cur->UpdateBranchAABB();
      cur = cur->parent;
    }
  }

  if (deleteNode) {
    MemoryManager::DeleteOnFreeList<Node>(node);
  }
}

void BVTree::DebugDraw() const {
  std::queue<Node*> q;

  if (root != nullptr) {
    q.push(root);
  }

  while (!q.empty()) {
    Node* cur = q.front();

    Color color;
    if (cur->IsLeaf()) {
      if (collisionSet.find(cur->collider) != collisionSet.end()) {
        color = Color::red;
      } else {
        color = Color::green;
      }
      DebugDraw::WireCube(Math::Matrix4::Translate(cur->aabb.GetCenter()) *
                              Math::Matrix4::Scale({cur->aabb.GetSize()}),
                          color, 1, .05);
    } else {
      int depth = 0;
      auto parent = cur->parent;
      while (parent != nullptr) {
        depth++;
        parent = parent->parent;
      }
      color = Color::Lerp(Color::white, Color::black,
                          static_cast<float>(depth) / 10);
      DebugDraw::WireCube(Math::Matrix4::Translate(cur->aabb.GetCenter()) *
                              Math::Matrix4::Scale({cur->aabb.GetSize()}),
                          color, 1, .05);
    }

    q.pop();
    if (cur->left != nullptr) {
      q.push(cur->left);
    }
    if (cur->right != nullptr) {
      q.push(cur->right);
    }
  }
}

bool BVTree::Raycast(const Ray& ray, RaycastHit* const hitInfo,
                     float maxDistance) {
  return Raycast(root, ray, hitInfo, maxDistance);
}
bool BVTree::Raycast(Node* const node, const Ray& ray,
                     RaycastHit* const hitInfo, float maxDistance) {
  if (node == nullptr || !node->aabb.Raycast(ray, nullptr, maxDistance))
    return false;
  if (node->IsLeaf()) {
    RaycastHit hitTmp{};
    if (node->collider->Raycast(ray, &hitTmp, maxDistance) &&
        hitTmp.GetDistance() < hitInfo->GetDistance()) {
      *hitInfo = std::move(hitTmp);
      return true;
    } else
      return false;
  } else {
    return Raycast(node->left, ray, hitInfo, maxDistance) ||
           Raycast(node->right, ray, hitInfo, maxDistance);
  }
}

const CollisionUtil::ColliderPairSet& BVTree::GetCollisionPairs() {
  PROFILE
  colliderPairSet.clear();
#if _EDITOR
  collisionSet.clear();
#endif

  for (const auto& pair : colNodeMap) {
    if (pair.first->GetProperty(Collider::Property::IS_STATIC)) continue;

    Collider* curCollider = pair.first;
    AABB aabb = curCollider->GetFatAABB();
    std::queue<Node*> q;

    if (root != nullptr) {
      q.push(root);
    }

    while (!q.empty()) {
      Node* curNode = q.front();
      q.pop();

      if (curNode->IsLeaf()) {
        Collider* col = curNode->collider;
        if (curCollider != col) {
          colliderPairSet.insert({curCollider, curNode->collider});
#if _EDITOR
          collisionSet.insert(curCollider);
          collisionSet.insert(curNode->collider);
          DebugDraw::Line(curCollider->GetWorldCenter(),
                          curNode->collider->GetWorldCenter(), Color::blue, 1, .05);
#endif
        }
      } else {
        if (curNode->left->aabb.Intersect(aabb)) {
          q.push(curNode->left);
        }
        if (curNode->right->aabb.Intersect(aabb)) {
          q.push(curNode->right);
        }
      }
    }
  }

  return colliderPairSet;
}

void BVTree::Node::UpdateBranchAABB() {
  ASSERT(collider == nullptr && !IsLeaf());
  aabb = AABB::Encapsulate(left->aabb, right->aabb);
}

void BVTree::Node::UpdateLeafAABB() {
  PROFILE
  ASSERT(IsLeaf() && collider != nullptr);
  aabb = collider->GetFatAABB();
}

void BVTree::Node::SwapOutChild(Node* const oldChild, Node* const newChild) {
  ASSERT(oldChild == left || oldChild == right);
  if (oldChild == left) {
    left = newChild;
    left->parent = this;
  } else {
    right = newChild;
    right->parent = this;
  }
}

}  // namespace Isetta