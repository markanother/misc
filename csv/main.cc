#include <iostream>
#include <string>
#include <cstdint>
#include <fstream>
#include <vector>
#include <cassert>

inline bool isspace(char c) {
  return (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f');
}

inline bool isdigit(char c) {
  return (c >= '0' && c <= '9');
}

inline float strtof(const char *nptr, char **endptr) {
  const char *p = nptr;
  // Skip leading white space, if any. Not necessary
  while (isspace(*p) ) ++p;

  // Get sign, if any.
  bool sign = true;
  if (*p == '-') {
    sign = false; ++p;
  } else if (*p == '+') {
    ++p;
  }

  // Get digits before decimal point or exponent, if any.
  float value;
  for (value = 0; isdigit(*p); ++p) {
    value = value * 10.0f + (*p - '0');
  }

  // Get digits after decimal point, if any.
  if (*p == '.') {
    uint64_t pow10 = 1;
    uint64_t val2 = 0;
    ++p;
    while (isdigit(*p)) {
      val2 = val2 * 10 + (*p - '0');
      pow10 *= 10;
      ++p;
    }
    value += static_cast<float>(
        static_cast<double>(val2) / static_cast<double>(pow10));
  }

  // Handle exponent, if any.
  if ((*p == 'e') || (*p == 'E')) {
    ++p;
    bool frac = false;
    float scale = 1.0;
    unsigned expon;
    // Get sign of exponent, if any.
    if (*p == '-') {
      frac = true;
      ++p;
    } else if (*p == '+') {
      ++p;
    }
    // Get digits of exponent, if any.
    for (expon = 0; isdigit(*p); p += 1) {
      expon = expon * 10 + (*p - '0');
    }
    if (expon > 38) expon = 38;
    // Calculate scaling factor.
    while (expon >=  8) { scale *= 1E8;  expon -=  8; }
    while (expon >   0) { scale *= 10.0; expon -=  1; }
    // Return signed and scaled floating point result.
    value = frac ? (value / scale) : (value * scale);
  }

  if (endptr) *endptr = (char*)p;  // NOLINT(*)
  return sign ? value : - value;
}

typedef float real_t;

template<typename DType>
class DataIter {
 public:
  virtual ~DataIter(void) {}
  
  virtual void Reset(void) = 0;

  virtual bool Next(void) = 0;

  virtual DType &Value(void) = 0;
};


class Row {
 public:
  real_t *label;

  size_t num_dim;

  real_t *value;

  inline real_t get_value(size_t i)  {
    return value[i];
  }

  inline real_t get_label()  {
    return *label;
  }

  inline real_t SDot(const std::vector<real_t>& weight)  {
    assert(weight.size() == num_dim);
    real_t sum = 0;
    for (size_t i = 0; i < num_dim; ++i) {
      sum += weight[i] * value[i];
    }    
    return sum;
  }
};


struct RowBlock {
  size_t size;
  size_t num_dim;

  real_t *label;
  real_t *value;

  inline Row operator[](size_t rowid) {
    assert(rowid < size);
    Row inst;
    inst.label = label + rowid;
    inst.value = value + rowid * num_dim;
    inst.num_dim = num_dim;
 
    return inst;
  }
};

class CSVIter : public DataIter<RowBlock> {
 public:
  CSVIter(RowBlock *block, std::string& filename, int label_col, size_t batch_size) : 
    block_(block), batch_size_(batch_size), label_col_(label_col) {
    input_.open(filename.c_str(), std::ios::in);
  }

  void Reset(void) {
    input_.seekg(0, std::ios::beg);
  }

  bool Next(void) {
    size_t size = 0;
    std::string line;
    std::string strBlock;
    while (getline(input_, line)) {
      strBlock += line + "\n";
      size++;
      if (size == batch_size_) break;
    }
    if (size == 0) return false;
    std::cout << strBlock << std::endl;
    block_->size = size;
    ParseBlock((char *)strBlock.c_str(), (char *)strBlock.c_str() + strBlock.length()-1);
    return true;
  }

  RowBlock &Value(void) {
    return *block_;
  }

  void ParseBlock(char *begin, char *end) {
    char *lbegin = begin;
    char *lend = lbegin;
    size_t size = 0;
    real_t *value = block_->value;
    real_t *label = block_->label;
  
    while (lbegin != end) {
      // get line end
      lend = lbegin + 1;
      while (lend != end && *lend != '\n' && *lend != '\r') ++lend;
      
      size++;
  
      char* p = lbegin;
      int column_index = 0;
      size_t idx = 0;
  
      while (p != lend) {
        char *endptr;
        float v = strtof(p, &endptr);
        p = endptr;
  
        //std::cout << ++idx << std::endl;
        //std::cout << v << std::endl;
        if (column_index == label_col_){
          *label = v;
          label++;
          std::cout << v << " label" << std::endl;
        } else {
          *value = v;
          value++;
          std::cout << v << " value" << std::endl;
        }
  
        ++column_index;
        while (*p != ',' && p != lend) ++p;
        if (p != lend) ++p;
      }
      // skip empty line
      while ((*lend == '\n' || *lend == '\r') && lend != end) ++lend;
      lbegin = lend;
    }
  
    block_->size = size;
  }


  RowBlock *block_;
  int label_col_;
  std::ifstream input_;
  size_t batch_size_;
};


/*
void ParseBlock(char *begin,
           char *end) {
  char * lbegin = begin;
  char * lend = lbegin;
  int s=0;
  while (lbegin != end) {
    // get line end
    lend = lbegin + 1;
    while (lend != end && *lend != '\n' && *lend != '\r') ++lend;
    s++;
    std::cout<<s<<std::endl;
    char* p = lbegin;
    int column_index = 0;
    size_t idx = 0;
    float label = 0.0f;

    while (p != lend) {
      char *endptr;
      float v = strtof(p, &endptr);
      p = endptr;

      //std::cout << ++idx << std::endl;
      //std::cout << v << std::endl;

      ++column_index;
      while (*p != ',' && p != lend) ++p;
      if (p != lend) ++p;
    }
    // skip empty line
    while ((*lend == '\n' || *lend == '\r') && lend != end) ++lend;
    lbegin = lend;
  }
}
*/

int main(){
  /*
  std::string s = "hello world\n";
  const char *c = s.c_str();
  std::cout << s.length() << std::endl;
  for (int i=0; i<s.length(); ++i ){
     std::cout << *c << std::endl;
     ++c;
  }
  */
  /*  
  std::string s = "40,,60\n\n8,9,10\n\n";
  char *c = (char*)s.c_str();
  ParseBlock(c, c+s.length());
  */
  /*
  std::string filename = "data.csv";
  std::ifstream input(filename.c_str());
  std::string line;
  while(getline(input, line)) std::cout << line << std::endl;
  */
  real_t *val = new real_t[6];
  real_t *lbl = new real_t[2];
  RowBlock blk;
  blk.size = 2;
  blk.num_dim = 3;
  blk.value = val;
  blk.label = lbl;

  std::string name = "data.csv";
  CSVIter csv_iter(&blk, name, 2, (size_t)2);
  while(csv_iter.Next()){
    RowBlock block = csv_iter.Value();
    std::cout << block[1].get_value(1) << std::endl;
    std::cout << block[1].get_label() << std::endl;
  }
  return 0;
}
