// $Id: IndexEntry.h,v 1.7 2010-06-11 13:05:30 mjohn Exp $
#ifndef INDEXENTRY_H 
#define INDEXENTRY_H 1

// Include files
#include <vector>
#include <string>

/** @class IndexEntry IndexEntry.h
 *  
 *
 *  @author Malcolm John
 *  @date   2009-07-06
 */
class IndexEntry {
 public: 
  IndexEntry(){}
  IndexEntry(int r,double t, std::string s, std::string d, std::string n, int e){m_roughtime=r;m_time=t;m_source=s;m_detector=d,m_file=n;m_entry=e;}
  virtual ~IndexEntry( ){}
  int roughtime() const {return m_roughtime;}
  double time() const {return m_time;}
  std::string file(){return m_file;}
  int entry(){return m_entry;}
  int mode(){return m_entry;}
  std::string source(){return m_source;}
  std::string detector(){return m_detector;}
 private:
  std::string m_source;
  std::string m_detector;
  std::string m_file;
  int m_roughtime;
  double m_time;
  int m_entry;
};

typedef std::vector<IndexEntry> Index;

#endif // INDEXENTRY_H
