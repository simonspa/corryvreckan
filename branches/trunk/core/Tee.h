#ifndef TEE_H
#define TEE_H 1

//-------------------------------------------------------------------------------
// Simple class to replace cout. It adds the algorithm name to the beginning of
// every new line.
//-------------------------------------------------------------------------------

class tee{

public:
  
  tee(){m_newLine=true;}
  tee(std::string algorithmName){
    m_algorithmName = algorithmName;
  }
  ~tee(){}
  
  template<class T>
  tee &operator<<(const T &x)
  {
    if(m_newLine){
      std::cout<<"["<<m_algorithmName<<"] "<<x;
      m_newLine = false;
    }else std::cout<<x;
    return *this;
  }
  
  tee& operator<<(std::ostream& (*fun)(std::ostream&)){
    std::cout << std::endl;
    m_newLine=true;
  }
  
  // Member variables - the algorithm name and if it is the beginning of a new line
  std::string m_algorithmName;
  bool m_newLine;
  
};

#endif
