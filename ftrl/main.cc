#include "csv_iter.h"

#include <iostream>


int main(){
  real_t *val = new real_t[6];
  real_t *lbl = new real_t[2];
  RowBlock blk;
  blk.size = 2;
  blk.num_dim = 2;
  blk.value = val;
  blk.label = lbl;

  std::string name = "data.csv";
  CSVIter csv_iter(&blk, name, 2, (size_t)2);
  while(csv_iter.Next()){
    RowBlock block = csv_iter.Value();
    std::cout << block[0].get_value(1) << std::endl;
    std::cout << block[0].get_label() << std::endl;
  }
  return 0;
}
