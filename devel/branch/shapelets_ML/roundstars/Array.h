//---------------------------------------------------------------------------
#ifndef ArrayH
#define ArrayH
//---------------------------------------------------------------------------

#include <vector>
#include <assert.h>

template <class T>
class Array {

  public:

    Array(size_t _n1=0,size_t _n2=0) : n1(_n1),n2(_n2),data(_n1,std::vector<T>(_n2)) {}
    Array(size_t _n1,size_t _n2,const T& value) : n1(_n1),n2(_n2),
    data(_n1,std::vector<T>(_n2,value)) {}
    ~Array() {}
    Array(const Array& rhs) : n1(rhs.n1),n2(rhs.n2),data(rhs.data) {}
    Array<T>& operator=(const Array<T>& rhs)
    { 
      if (this == &rhs) return *this; n1 = rhs.n1; n2 = rhs.n2; 
      data = rhs.data; return *this; 
    }
    std::vector<T>& operator[](size_t i) { return data[i]; }
    const std::vector<T>& operator[](size_t i) const {return data[i];}
    T& operator()(size_t i, size_t j) {return data[i][j];}
    const T& operator()(size_t i, size_t j) const {return data[i][j];}
    size_t GetN1() const {assert(data.size()==n1); return n1;}
    size_t GetN2() const {assert(data.size()==0 || data[0].size()==n2); return n2;}
    const T& Get(size_t i,size_t j) const 
    { assert(i<GetN1()); assert(j<GetN2()); return data[i][j]; }
    const std::vector<T>& GetRow(size_t i) const
    { assert(i<GetN1()); return data[i]; }
    std::vector<T> GetCol(size_t j) const
    { 
      assert(j<GetN2()); 
      std::vector<T> temp(GetN1()); 
      for(size_t i=0;i<GetN1();i++) temp[i] = data[i][j]; 
      return temp; 
    }
    void SetN1(size_t newn1);
    void SetN2(size_t newn2);
    void SetAllValues(const T& value)
    {
      for(size_t i=0;i<GetN1();i++) for(size_t j=0;j<GetN2();j++) 
	data[i][j] = value; 
    }
    void Set(size_t i,size_t j,const T& value) {data[i][j] = value;}
    void SetRow(size_t i,const std::vector<T>& row) 
    {
      assert(i<GetN1());
      assert(row.size()==GetN2());
      for(size_t j=0;j<GetN2();j++) data[i][j] = row[j]; 
    }
    void SetCol(size_t j,const std::vector<T>& col)
    { 
      assert(j<GetN2());
      assert(col.size()==GetN1());
      for(size_t i=0;i<GetN1();i++) data[i][j] = col[i]; 
    }
    void SetRow(size_t i,const std::vector<T>& row,size_t start,size_t end) 
    {
      assert(i<GetN1());
      assert(start <= end);
      assert(end < row.size());
      assert(end-start+1==GetN2());
      for(size_t j=0;j<GetN2();j++) data[i][j] = row[j+start]; 
    }
    void SetCol(size_t j,const std::vector<T>& col,size_t start,size_t end)
    {
      assert(j<GetN2());
      assert(start <= end);
      assert(end < col.size());
      assert(end-start+1==GetN1());
      for(size_t i=0;i<GetN1();i++) data[i][j] = col[i+start]; 
    }
    void AddRow(const std::vector<T>& row) 
    { 
      assert(row.size()==GetN2());
      data.push_back(row); n1++; 
    }
    void AddCol(const std::vector<T>& col) 
    { 
      assert(col.size()==GetN1());
      for(size_t i=0;i<GetN1();i++) data[i].push_back(col[i]); n2++; 
    }
    void Reserve(size_t n1r,size_t n2r);

  protected :

    size_t n1,n2;
    std::vector<std::vector<T> > data;
};

template <class T>
inline void Array<T>::SetN1(size_t newn1)
{
  if (n1 == 0) data = std::vector<std::vector<T> >(newn1,std::vector<T>(n2));
  else if (newn1 > n1) data.insert(data.end(),newn1-n1,std::vector<T>(n2));
  else data.erase(data.begin()+newn1,data.end());
  n1 = newn1;
}

template <class T>
inline void Array<T>::SetN2(size_t newn2)
{
  if (n2 == 0) for(size_t i=0;i<n1;i++) data[i] = std::vector<T>(newn2);
  else if (newn2 > n2) for(size_t i=0;i<n1;i++)
      data[i].insert(data[i].end(),newn2-n2,0.);
  else for(size_t i=0;i<n1;i++)
      data[i].erase(data[i].begin()+newn2,data[i].end());
  n2 = newn2;
}

template <class T>
inline void Array<T>::Reserve(size_t n1r,size_t n2r)
{
  if (n1r > n1) data.reserve(n1r);
  if (n2r > n2) for(size_t i=0;i<n1;i++) data[i].reserve(n2r);
}

#endif
