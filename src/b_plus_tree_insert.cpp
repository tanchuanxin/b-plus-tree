#include "b_plus_tree.h"
#include "types.h"

#include <vector>
#include <cstring>
#include <iostream>

using namespace std;

// Insert a record into the B+ Tree index. Key: Record's avgRating, Value: {blockAddress, offset}.
void BPlusTree::insert(Address address, float key)
{
  std::cerr << "Inserting: " << key << endl;
  // If no root exists, create a new B+ Tree root.
  if (rootAddress == nullptr)
  {
    // Create new node in main memory, set it to root, and add the key and values to it.
    root = new Node(maxKeys);
    root->keys[0] = key;
    root->isLeaf = true; // It is both the root and a leaf.
    root->numKeys = 1;
    root->pointers[0] = address; // Add record's disk address to pointer.

    // Write the root node into disk.
    void *rootDiskAddress = index->allocate(nodeSize).blockAddress;
    std::memcpy(rootDiskAddress, root, nodeSize);

    // Keep track of root node's disk address.
    rootAddress = rootDiskAddress;

    // Update number of nodes and levels
    numNodes++;
    levels++;

    std::cerr << "(root) Inserted: " << key << endl;
  }
  // Else if root exists already, traverse the nodes to find the proper place to insert the key.
  else
  {
    // Load in root from the disk
    void *rootMainMemory = operator new(nodeSize);
    std::memcpy(rootMainMemory, rootAddress, nodeSize);
    root = (Node *)rootMainMemory;

    Node *cursor = root;
    Node *parent;                          // Keep track of the parent as we go deeper into the tree in case we need to update it.
    void *parentDiskAddress = rootAddress; // Keep track of parent's disk address as well so we can update parent in disk.
    void *cursorDiskAddress = rootAddress; // Store current node's disk address in case we need to update it in disk.

    for (int i = 0; i < cursor->numKeys; i++)
    {
      std::cerr << cursor->keys[i] << " | ";
    }
    std::cerr << endl;

    // While not leaf, keep following the nodes to correct key.
    while (cursor->isLeaf == false)
    {

      // Set the parent of the node (in case we need to assign new child later), and its disk address.
      parent = cursor;
      parentDiskAddress = cursorDiskAddress;

      std::cerr << "Finding where to put in key " << key << endl;
      for (int i = 0; i < cursor->numKeys; i++)
      {
        std::cerr << cursor->keys[i] << " | ";
      }

      std::cerr << endl;

      for (int i = 0; i < cursor->numKeys + 1; i++)
      {
        std::cerr << cursor->pointers[i].blockAddress << " | ";
      }

      std::cerr << endl;

      // Check through all keys of the node to find key and pointer to follow downwards.
      for (int i = 0; i < cursor->numKeys; i++)
      {
        // If key is lesser than current key, go to the left pointer's node.
        if (key < cursor->keys[i])
        {
          // Load node in from disk to main memory.
          void *mainMemoryNode = operator new(nodeSize);
          std::memcpy(mainMemoryNode, cursor->pointers[i].blockAddress, nodeSize);

          // Update cursorDiskAddress to maintain address in disk if we need to update nodes.
          cursorDiskAddress = cursor->pointers[i].blockAddress;

          std::cerr << "Going to address: " << cursor->pointers[i].blockAddress;

          // Move to new node in main memory.
          cursor = (Node *)mainMemoryNode;

          break;
        }
        // Else if key larger than all keys in the node, go to last pointer's node (rightmost).
        if (i == cursor->numKeys - 1)
        {
          // Load node in from disk to main memory.
          void *mainMemoryNode = operator new(nodeSize);
          std::memcpy(mainMemoryNode, cursor->pointers[i + 1].blockAddress, nodeSize);

          // Update diskAddress to maintain address in disk if we need to update nodes.
          cursorDiskAddress = cursor->pointers[i + 1].blockAddress;

          std::cerr << "Going to address: " << cursor->pointers[i + 1].blockAddress;

          // Move to new node in main memory.
          cursor = (Node *)mainMemoryNode;
          break;
        }
      }
    }

    // When we reach here, it means we have hit a leaf node. Let's find a place to put our new record in.
    // If this leaf node still has space to insert a key, then find out where to put it.
    if (cursor->numKeys < maxKeys)
    {
      std::cerr << "Node to insert in: " << cursor->keys[0] << endl;

      std::cerr << "Insert - child key into existing node:" << endl;
      for (int i = 0; i < cursor->numKeys; i++)
      {
        std::cerr << cursor->keys[i] << " | ";
      }
      std::cerr << endl;

      std::cerr << "Existing node's pointers:" << endl;
      for (int i = 0; i < cursor->numKeys + 1; i++)
      {
        std::cerr << cursor->pointers[i].blockAddress << " | ";
      }
      std::cerr << endl;

      int i = 0;
      // While we haven't reached the last key and the key we want to insert is larger than current key, keep moving forward.
      while (key > cursor->keys[i] && i < cursor->numKeys)
      {
        i++;
      }

      // Update the last pointer to point to the previous last pointer's node. Aka maintain cursor -> Y linked list.
      Address next = cursor->pointers[cursor->numKeys];

      // Now i represents the index we want to put our key in. We need to shift all keys in the node back to fit it in.
      // Swap from number of keys + 1 (empty key) backwards, moving our last key back and so on. We also need to swap pointers.
      for (int j = cursor->numKeys; j > i; j--)
      {
        // Just do a simple bubble swap from the back to preserve index order.
        cursor->keys[j] = cursor->keys[j - 1];
        cursor->pointers[j] = cursor->pointers[j - 1];
      }

      // Insert our new key and pointer into this node.
      cursor->keys[i] = key;
      cursor->pointers[i] = address;
      cursor->numKeys++;

      std::cerr << "Insert - After inserting keys:" << endl;
      for (int i = 0; i < cursor->numKeys; i++)
      {
        std::cerr << cursor->keys[i] << " | ";
      }
      std::cerr << endl;

      std::cerr << "After inserting pointers:" << endl;
      for (int i = 0; i < cursor->numKeys + 1; i++)
      {
        std::cerr << cursor->pointers[i].blockAddress << " | ";
      }
      std::cerr << endl;

      // Update leaf node pointer link to next node
      cursor->pointers[cursor->numKeys] = next;

      displayNode(cursor);

      std::cerr << "(not root) Inserted: " << key << endl;

      // Now insert operation is complete, we need to store this updated node to disk.
      // cursorDiskAddress is the address of node in disk, cursor is the address of node in main memory.
      // In this case, we count read/writes as 1/O only (Assume block remains in main memory).
      std::memcpy(cursorDiskAddress, cursor, nodeSize);
    }
    // Overflow: If there's no space to insert new key, we have to split this node into two and update the parent if required.
    else
    {
      std::cerr << "Overflow! Splitting..." << endl;

      // Create a new leaf node to put half the keys and pointers in.
      Node *newLeaf = new Node(maxKeys);

      // Update nodes count
      numNodes++;

      // Copy all current keys and pointers (including new key to insert) to a temporary list.
      float tempKeyList[maxKeys + 1];

      // We only need to store pointers corresponding to records (ignore those that points to other nodes).
      // Those that point to other nodes can be manipulated by themselves without this array later.
      Address tempPointerList[maxKeys + 1];
      Address next = cursor->pointers[cursor->numKeys];

      // Copy all keys and pointers to the temporary lists.
      int i = 0;
      for (i = 0; i < maxKeys; i++)
      {
        tempKeyList[i] = cursor->keys[i];
        tempPointerList[i] = cursor->pointers[i];
      }

      // Insert the new key into the temp key list, making sure that it remains sorted. Here, we find where to insert it.
      i = 0;
      while (key > tempKeyList[i] && i < maxKeys)
      {
        i++;
      }

      // The key should be inserted at index i in the temporary lists. Move all elements back.
      for (int j = maxKeys; j > i; j--)
      {
        // Bubble swap all elements (keys and pointers) backwards by one index.
        tempKeyList[j] = tempKeyList[j - 1];
        tempPointerList[j] = tempPointerList[j - 1];
      }

      // Insert the new key and pointer into the temporary lists.
      tempKeyList[i] = key;
      tempPointerList[i] = address;

      newLeaf->isLeaf = true; // New node is a leaf node.

      // Split the two new nodes into two. ⌊(n+1)/2⌋ keys for left, n+1 - ⌊(n+1)/2⌋ (aka remaining) keys for right.
      cursor->numKeys = (maxKeys + 1) / 2;
      newLeaf->numKeys = (maxKeys + 1) - ((maxKeys + 1) / 2);

      // Set the last pointer of the new leaf node to point to the previous last pointer of the existing node (cursor).
      // Essentially newLeaf -> Y, where Y is some other leaf node pointer wherein cursor -> Y previously.
      // We use maxKeys since cursor was previously full, so last pointer's index is maxKeys.
      newLeaf->pointers[newLeaf->numKeys] = next;

      // Set the new last pointer of the existing cursor to point to the new leaf node (linked list).
      // Effectively, it was cursor -> Y, now it's cursor -> newLeaf -> Y, where Y is some other leaf node.
      // We need to save the new leaf node to the disk and store that disk address in the pointer.
      Address newLeafAddress = index->allocate(nodeSize);
      cursor->pointers[cursor->numKeys] = newLeafAddress;

      // Now we need to deal with the rest of the keys and pointers.
      // Note that since we are at a leaf node, pointers point directly to records on disk.

      // Add in keys and pointers in both the existing node, and the new leaf node.
      // First, the existing node (cursor).
      for (i = 0; i < cursor->numKeys; i++)
      {
        cursor->keys[i] = tempKeyList[i];
        cursor->pointers[i] = tempPointerList[i];
      }

      // Then, the new leaf node. Note we keep track of the i index, since we are using the remaining keys and pointers.
      for (int j = 0; j < newLeaf->numKeys; i++, j++)
      {
        newLeaf->keys[j] = tempKeyList[i];
        newLeaf->pointers[j] = tempPointerList[i];
      }

      // Now that we have finished updating the two new leaf nodes, we need to write them to disk.
      std::memcpy(cursorDiskAddress, cursor, nodeSize);

      std::memcpy(newLeafAddress.blockAddress, newLeaf, nodeSize);

      // If we are at root (aka root == leaf), then we need to make a new parent root.
      if (cursor == root)
      {
        Node *newRoot = new Node(maxKeys);

        // We need to set the new root's key to be the left bound of the right child.
        newRoot->keys[0] = newLeaf->keys[0];

        // Point the new root's children as the existing node and the new node.
        Address cursorDisk{cursorDiskAddress, 0};

        newRoot->pointers[0] = cursorDisk;
        newRoot->pointers[1] = newLeafAddress;

        // Update new root's variables.
        newRoot->isLeaf = false;
        newRoot->numKeys = 1;

        // Add new node and level to the tree.
        numNodes++;
        levels++;

        // Write the new root node to disk and update the root disk address stored in B+ Tree.
        Address newRootAddress = index->allocate(nodeSize);
        std::memcpy(newRootAddress.blockAddress, newRoot, nodeSize);

        rootAddress = newRootAddress.blockAddress;
        root = newRoot;

        std::cerr << "Making new root:" << newLeaf->keys[0] << endl;
      }
      // If we are not at the root, we need to insert a new parent in the middle levels of the tree.
      else
      {
        insertInternal(newLeaf->keys[0], (Node *)parentDiskAddress, (Node *)newLeafAddress.blockAddress);
      }
    }
  }
}