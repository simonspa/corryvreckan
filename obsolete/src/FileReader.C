#include <sstream>
#include "FileReader.h"

// boost
//#include "boost/filesystem.hpp"
//using namespace boost::filesystem;

//********************************************************************************
FileReader::FileReader(const char* filename)
	:  m_file( new std::ifstream(filename, std::ios::in)), m_line(), m_filename(filename), m_failed(false)
{
    //counter();
}

//********************************************************************************
FileReader::FileReader(const std::string& filename)
	: m_file( new std::ifstream(filename.c_str(), std::ios::in)), m_line(), m_filename(filename), m_failed(false)
{
    //counter();
}	

FileReader::~FileReader() {

  m_file.get()->close();
  
}

//********************************************************************************
void FileReader::counter()
{
    printf("m_file now has %li references\n", m_file.use_count());
}

//********************************************************************************
bool FileReader::isValid() const
{
	//path myfile(m_filename);
    // what do you want to do if the file doesn't exist 
    if( m_file.get()->fail() )
    {
		return false;
    }
    else
		return true;
}

//********************************************************************************
void FileReader::setFileName( const std::string& fileName)
{
    m_filename = fileName;
    m_file = boost::shared_ptr<std::ifstream>( new std::ifstream(fileName.c_str(), std::ios::in));
    //counter();
}

//********************************************************************************
void FileReader::setline( const std::string& line)
{
    m_line = line;
}

//********************************************************************************
void FileReader::setisValid( const bool& failed)
{
    m_failed = failed;
}

//********************************************************************************
FileReader& FileReader::operator=(const FileReader& other)
{
    if ( this != &other ) // Ignore attepmts at self-assignment
    {
        m_file = other.getFile();
        m_line = other.getLine();
        m_filename = other.getFileName();
        m_failed = other.inputFailed();
    }
    return *this;
}

//********************************************************************************
bool FileReader::nextLine()
{
    getline(*m_file.get(), m_line);
    if ( m_file.get()->eof() )
        return false;
    else return true;
}

//********************************************************************************
void FileReader::skip_fields(std::istringstream& ist, const int n)
{
    if ( n < 1 )
        return;
    std::string tmp;
    for(int i = 1; i <= n; ++i)
    {
        ist >> tmp;
    }
}

//********************************************************************************
int FileReader::getFieldAsInt(const int n) 
{	
    m_failed = false;
    std::istringstream ist(m_line);
    this->skip_fields(ist, n-1);
    int rval;
    ist >> rval;
    if ( ist.fail() )
    {
        m_failed = true;
        return 0;
    }
    else
        return rval;
}

//********************************************************************************
float FileReader::getFieldAsFloat(const int n)
{
    m_failed = false;
    std::istringstream ist(m_line);
    this->skip_fields(ist, n-1);
    float rval;
    ist >> rval;
    if ( ist.fail() )
    {
        m_failed = true;
        return 0.0;
    }
    else
        return rval;
}

//********************************************************************************
double FileReader::getFieldAsDouble(const int n)
{
    m_failed = false;
    std::istringstream ist(m_line);
    this->skip_fields(ist, n-1);
    double rval;
    ist >> rval;
    if ( ist.fail() )
    {
        m_failed = true;
        return 0.0;
    }
    else
        return rval;
}


//********************************************************************************
std::string FileReader::getFieldAsString(const int n)
{
    m_failed = false;
    std::istringstream ist(m_line);
    this->skip_fields(ist, n-1);
    std::string rval;
    ist >> rval;
    if ( ist.fail() )
    {
        m_failed = true;
        return std::string();
    }
    else
        return rval;
}
