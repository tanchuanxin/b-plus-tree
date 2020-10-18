#include "memory_pool.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <unordered_map>

using namespace std;

int main()
{
  MemoryPool db(500000000, 100);

  // =============================================================
  // Experiment 1:
  // Store the data (which is about IMDb movives and described in Part 4) on the disk and report the following statistics:
  // - The number of blocks;
  // - The size of database;
  // =============================================================

  // Create a list of all addresses
  std::vector<std::tuple<void *, std::size_t>> records;

  // Open test data
  std::ifstream file("../data/testdata.tsv");

  // Insert data into database and populate list of addresses
  if (file.is_open())
  {
    std::string line;
    int recordNum = 0;
    while (std::getline(file, line))
    {
      Record temp;
      stringstream linestream(line);
      string data;

      strcpy(temp.tconst, line.substr(0, line.find("\t")).c_str());

      std::getline(linestream, data, '\t');
      linestream >> temp.averageRating >> temp.numVotes;

      std::tuple<void *, std::size_t> record = db.allocate(sizeof(temp));

      // Add it to list of addresses
      records.push_back(record);

      // Add to database
      memcpy(std::get<0>(record) + std::get<1>(record), &temp, sizeof(temp));

      cout << "Inserted record " << recordNum + 1 << " at address: " << std::get<0>(record) + std::get<1>(record) << '\n';

      recordNum += 1;
    }
    file.close();
  };

  cout << "Number of blocks used: " << db.getAllocated() << " blocks" << '\n';
  cout << "Actual size used: " << db.getActualSizeUsed() << " bytes" << '\n';
  cout << "Total size occupied: " << db.getSizeUsed() << " bytes" << '\n';

  // =============================================================
  // Experiment 2:
  // Build a B+ tree on the attribute "averageRating" by inserting the records sequentially and report the following statistics:
  // - The parameter n of the B + tree;
  // - The number of nodes of the B + tree;
  // - The height of the B + tree, i.e., the number of levels of the B + tree;
  // - The root node and its child nodes(actual content);
  // =============================================================

  // Here, we are trying to fake the overhead of loading from disk to main memory.
  // Therefore, we copy out the data from disk to a heap in main memory.

  // Create a map to detect which blocks we have already loaded into main memory.
  // The key is the block's address (on disk), the value is the block's address (in main memory).
  unordered_map<void *, void *> loadedBlocks;

  // Iterate through all records stored in database.
  for (std::vector<std::tuple<void *, std::size_t>>::iterator it = records.begin(); it != records.end(); ++it)
  {
    // This mimics the loading of disk data to main memory, pointing at the disk block.
    void *blockAddress = std::get<0>(*it);
    std::size_t offset = std::get<1>(*it);

    // Check if this block hasn't been loaded yet.
    if (loadedBlocks.find(blockAddress) == loadedBlocks.end())
    {
      void *mainMemoryBlock = operator new(db.getBlockSize());
      memcpy(mainMemoryBlock, blockAddress, db.getBlockSize());

      loadedBlocks[blockAddress] = mainMemoryBlock;
    }

    // If block has been loaded, just read it.
    void *recordAddress = loadedBlocks.at(blockAddress) + offset;

    std::cout << (*(Record *)recordAddress).tconst << " at " << recordAddress << '\n';
  }

  // =============================================================
  // Experiment 3:
  // Retrieve the attribute “tconst” of those movies with the "averageRating" equal to 8 and report the following statistics:
  // - The number and the content of index nodes the process accesses;
  // - The number and the content of data blocks the process accesses;
  // - The attribute “tconst” of the records that are returned;
  // =============================================================

  // =============================================================
  // Experiment 3:
  // Retrieve the attribute “tconst” of those movies with the "averageRating" equal to 8 and report the following statistics:
  // - The number and the content of index nodes the process accesses;
  // - The number and the content of data blocks the process accesses;
  // - The attribute “tconst” of the records that are returned;
  // =============================================================

  // =============================================================
  // Experiment 4:
  // Retrieve the attribute “tconst” of those movies with the attribute “averageRating” from 7 to 9, both inclusively
  // and report the following statistics:
  // - The number and the content of index nodes the process accesses;
  // - The number and the content of data blocks the process accesses;
  // - The attribute “tconst” of the records that are returned;
  // =============================================================

  // =============================================================
  // Experiment 5:
  // Delete those movies with the attribute “averageRating” equal to 7, update the B + tree accordingly,
  // and report the following statistics:
  // - The number of times that a node is deleted(or two nodes are merged) during the process of the updating the B + tree;
  // - The number nodes of the updated B + tree;
  // - The height of the updated B + tree;
  // - The root node and its child nodes of the updated B + tree;
  // =============================================================

  return 0;
}