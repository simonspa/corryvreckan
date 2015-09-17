#ifndef BEAMTESTFILEREADER_HH
#define BEAMTESTFILEREADER_HH

#include <fstream>
#include <string>
#include "boost/shared_ptr.hpp"

class FileReader 
{
	public:
		FileReader();
		FileReader(const char* filename);
		FileReader(const std::string& filename);
    ~FileReader();
        // Copy assignment operator
        FileReader& operator=(const FileReader& other);
        
        // public member functions
        bool nextLine() ;
        int getFieldAsInt(const int n);
        float getFieldAsFloat(const int n);
        double getFieldAsDouble(const int n);
        std::string getFieldAsString(const int n);

        // setters
        void setFileName( const std::string& fileName);
        void setline( const std::string& line) ;
        void setisValid( const bool& failed) ;

        // getters
        std::ifstream* getFileif() const { return m_file.get(); };
        boost::shared_ptr<std::ifstream>getFile() const { return m_file; };
        std::string getFileName() const { return m_filename; };
        std::string getLine() const { return m_line; };
        bool inputFailed() const { return m_failed; };
        bool isValid() const;
    
    private:
        // private member functions
        void counter();
        void skip_fields(std::istringstream& ist, const int n);
        
        // private member variables
        boost::shared_ptr<std::ifstream> m_file;
        std::string m_line;
        std::string m_filename;
        bool m_failed;
};

#endif
