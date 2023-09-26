// WinFile.cpp : Testbed for the WinFile class.
//
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <conio.h>
#include <time.h>
#include <shlobj.h>
#include <vector>
#include "WinFile.h"
#include "HPFCounter.h"

#ifdef UNICODE
using std::wifstream;
using std::wofstream;
#define tcout     wcout
#define tcin      wcin
#define tifstream wifstream
#define tofstream wofstream
#define tstring   wstring
#else
using std::ifstream;
using std::ofstream;
#define tcout     cout
#define tcin      cin
#define tifstream ifstream
#define tofstream ofstream
#define tstring   string
#endif

//////////////////////////////////////////////////////////////////////////
//
// BASIC WRITE AND RE-READ TEST in text translation mode
//
//////////////////////////////////////////////////////////////////////////

void TestReadingWriting()
{
  std::tcout << _T("Writing the output file\n");
  std::tcout << _T("=======================\n");

//WinFile file;
  WinFile file(_T("C:\\TMP\\Testfile.txt"));
// file.SetFilenameByDialog(nullptr,true,"Test file","txt","Tempfile.txt");
//  file.CreateTempFileName("TMF","txt");

  CString testing(_T("This is a test for WinFile\n"));

  if(file.Open(winfile_write | FFlag::open_trans_text))
  {
    for(auto number = 0; number < 10; ++number)
    {
      file.Write(testing);
    }
    file.Close();
  }
  std::tcout << _T("Written.") << std::endl;

  // Now read it back
  std::tcout << _T("Reading back the file\n");
  std::tcout << _T("=====================\n");

  if (file.Open(winfile_read | FFlag::open_trans_text))
  {
    CString line;
    while(file.Read(line))
    {
      if(line.Compare(testing))
      {
        std::tcout << _T("ERROR reading test file back") << std::endl;
      }
    }
    file.Close();
  }
  std::tcout << _T("Done reading.") << std::endl;
  std::tcout.flush();


  if(file.CanAccess(true))
  {
    std::tcout << _T("The file [") << file.GetFilename().GetString() << _T("] exists and is writable") << std::endl;
  }
  file.DeleteToTrashcan();
  if(!file.Exists())
  {
    std::tcout << _T("The file [") << file.GetFilename().GetString() << _T("] is gone") << std::endl;
  }
}

//////////////////////////////////////////////////////////////////////////
//
// PERFORMANCE TEST TO COMPARE WITH fgets()
//
//////////////////////////////////////////////////////////////////////////

CString 
ReadWinFile(CString p_filename)
{
  CString result;
  CString line;

  WinFile file(p_filename);
  if (file.Open(winfile_read | FFlag::open_trans_text))
  {
    while (file.Read(line))
    {
      result += line;
    }
    file.Close();
  }
  return result;
}

#ifdef UNICODE
CString
ExplodeString(uchar* p_buffer,unsigned p_length)
{
  CString string;
  PWSTR buf = string.GetBufferSetLength(p_length + 1);
  for(unsigned index = 0;index < p_length; ++index)
  {
    *buf++ = (TCHAR) *p_buffer++;
  }
  *buf = (TCHAR) 0;
  string.ReleaseBufferSetLength(p_length);

  return string;
}
#endif

CString
TranslateBuffer(std::string p_string,Encoding p_encoding)
{
#ifdef UNICODE
  if(p_encoding == Encoding::UTF8)
  {
    // Convert UTF-8 -> UTF-16 -> MBCS
    int    length = (int) p_string.length() + 1;
    uchar* buffer = new uchar[length * 2];

    // Doing the 'real' conversion
    MultiByteToWideChar(65001 // FROM UTF-8 !!!
                       ,MB_PRECOMPOSED
                       ,p_string.c_str()
                       ,-1 // p_string.size()
                       ,reinterpret_cast<LPWSTR>(buffer)
                       ,length);
    CString result;
    int extra = 0;
    if(buffer[0] == 0xFF && buffer[1] == 0xFE)
    {
      extra = 2;
    }
    LPCTSTR resbuf = result.GetBufferSetLength(length * 2);
    memcpy((void*) resbuf,buffer+extra,length * 2 + 2);
    result.ReleaseBuffer();
    delete[] buffer;
    return result;
  }
  else if(p_encoding == Encoding::BE_UTF16)
  {
    // We are already UTF-16
    return CString((LPCTSTR) p_string.c_str());
  }
  // Last resort, create CString
  return ExplodeString(reinterpret_cast<uchar*>(const_cast<char*>(p_string.c_str())),(int)p_string.size());
#else
  if(p_encoding == Encoding::UTF8)
  {
    // Convert UTF-8 -> UTF-16 -> MBCS
    int   clength = 0;
    int    length = (int) p_string.length() + 2;
    uchar* buffer = new uchar[length * 2];

    // Doing the 'real' conversion
    clength = MultiByteToWideChar(65001 // FROM UTF-8 !!!
                                  ,MB_PRECOMPOSED
                                  ,p_string.c_str()
                                  ,-1 // p_string.size()
                                  ,reinterpret_cast<LPWSTR>(buffer)
                                  ,length);
    CString result;
    LPTSTR strbuf = result.GetBufferSetLength(clength);
    clength = ::WideCharToMultiByte(GetACP(),
                                    0,
                                    (LPCWSTR) buffer,
                                    -1, // p_length, 
                                    reinterpret_cast<LPSTR>(strbuf),
                                    clength,
                                    NULL,
                                    NULL);

    result.ReleaseBuffer();
    delete[] buffer;
    // ATLTRACE("BUF: %s\n",result.GetString());
    return result;
  }
  else if(p_encoding == Encoding::LE_UTF16)
  {
    // Implode to MBCS
    CString result;
    int   clength = 0;
    int    length = (int) p_string.length() + 2;
    LPTSTR buffer = result.GetBufferSetLength(length);

    clength = ::WideCharToMultiByte(GetACP(),
                                    0,
                                    (LPCWSTR) p_string.c_str(),
                                    -1, // p_length, 
                                    reinterpret_cast<LPSTR>(buffer),
                                    length,
                                    NULL,
                                    NULL);
    result.ReleaseBuffer();
    return result;
  }
  // Last resort, create CString
  return CString(p_string.c_str());
#endif
}


CString
ReadFILE(CString p_filename,Encoding p_encoding)
{
  std::string result;
  FILE* file = nullptr;
#ifdef UNICODE
  fopen_s(&file,CW2A(p_filename),"rt");
#else
  fopen_s(&file,p_filename.GetString(),"rt");
#endif
  if(file)
  {
    int ch;
    while((ch = fgetc(file)) != EOF)
    {
      result += (char) ch;
    }
    fclose(file);
  }
  Encoding p_type(Encoding::Default);
  unsigned skip = 0;
  WinFile::DefuseBOM((const uchar*) result.c_str(),p_type,skip);
  if(skip)
  {
    result = std::string(result.c_str() + skip);
  }
  return TranslateBuffer(result,p_encoding);
}

CString 
ReadWinFileStrings(CString p_filename)
{
  CString result;
  unsigned char buffer[1024];

  WinFile file(p_filename);
  if (file.Open(winfile_read | FFlag::open_trans_text))
  {
    while (file.Gets(buffer,1024))
    {
      // ATLTRACE("Buf: %s\n",buffer);
      result += (char*)buffer;
    }
    file.Close();
  }
  return result;
}

CString 
ReadFILEStrings(CString p_filename)
{
  CString result;
  char buffer[1024];

  FILE* file = nullptr;
#ifdef UNICODE
  fopen_s(&file,CW2A(p_filename),"rt");
#else
  fopen_s(&file, p_filename.GetString(), "rt");
#endif
  if (file)
  {
    while (fgets(buffer, 1024, file))
    {
      result += buffer;
    }
    fclose(file);
  }
  return result;
}

CString 
ReadIFSTREAMStrings(CString p_filename)
{
  std::tstring result;
  std::tstring line;

  tifstream file(p_filename);
  if (file.is_open())
  {
    while(std::getline(file,line))
    {
      result += line + _T('\n');
    }
    file.close();
  }
  return CString(result.c_str());
}

void
TestDifference(CString p_string1,CString p_string2)
{
  std::tcout << _T("Test difference of strings") << std::endl;

  int len1 = p_string1.GetLength();
  int len2 = p_string2.GetLength();

  if(len1 != len2)
  {
    std::tcout << _T("Lengths differ: [") << len1 << _T("] <-> [") << len2 << _T("]") << std::endl;
  }
  else
  {
    std::tcout << _T("Strings are of equal length: ") << len1 << std::endl;
  }
  for(int index = 0; index < len1; ++index)
  {
    if(p_string1.GetAt(index) != p_string2.GetAt(index))
    {
      std::tcout << _T("First different position: ") << index << std::endl;
      break;
    }
  }
}


void TestPerformance()
{
  HPFCounter measure;
  CString filename1 = _T("C:\\BIN\\KEYDB.cfg");
  CString filename2 = _T("C:\\BIN\\KEYDB_plain.cfg");
  CString filename3 = _T("C:\\BIN\\KEYDB_wide.cfg");
  CString filename4 = _T("C:\\BIN\\KEYDB_macos.cfg");

  std::tcout << _T("TESTING READ PERFORMANCE OF STRINGS") << std::endl;
  std::tcout << _T("\nTest 1: Reading UTF-8 with BOM") << std::endl;

  measure.Reset();
  measure.Start();
  CString firstfile = ReadWinFile(filename1);
  double result1 = measure.GetCounter();
  std::tcout << _T("Reading WinFile        : ") << result1 << std::endl;
  std::tcout << _T("Size of file 1         : ") << firstfile.GetLength() << std::endl;

  measure.Reset();
  measure.Start();
  CString secondfile = ReadFILE(filename1,Encoding::UTF8);
  double result2 = measure.GetCounter();
  std::tcout << _T("Reading <FILE>         : ") << result2 << std::endl;
  std::tcout << _T("Size of file 2         : ") << secondfile.GetLength() << std::endl;

  if(firstfile.Compare(secondfile))
  {
    std::tcout << _T("ALARM1: Read different file content!") << std::endl;
    TestDifference(firstfile,secondfile);
  }

  std::tcout << _T("\nTest 2: Reading ANSI text") << std::endl;

  measure.Reset();
  measure.Start();
  CString firstfile2 = ReadWinFile(filename2);
  result1 = measure.GetCounter();
  std::tcout << _T("Reading WinFile        : ") << result1 << std::endl;
  std::tcout << _T("Size of file 2         : ") << firstfile2.GetLength() << std::endl;

  measure.Reset();
  measure.Start();
  CString secondfile2 = ReadFILE(filename2,Encoding::Default);
  result2 = measure.GetCounter();
  std::tcout << _T("Reading <FILE>         : ") << result2 << std::endl;
  std::tcout << _T("Size of file 2 in ATL  : ") << secondfile2.GetLength() << std::endl;

  if (firstfile.Compare(secondfile))
  {
    std::tcout << _T("ALARM2: Read different file content!") << std::endl;
  }

  std::tcout << _T("\nTest 3: Reading ANSI text char-by-char") << std::endl;

  // Reading strings in our WinFile format
  measure.Reset();
  measure.Start();
  firstfile = ReadWinFileStrings(filename2);
  result1 = measure.GetCounter();
  std::tcout << _T("Reading strings WinFile: ") << result1 << std::endl;
  std::tcout << _T("Size of file 1         : ") << firstfile.GetLength() << std::endl;
  // Reading strings in standard <FILE> format
  measure.Reset();
  measure.Start();
  secondfile = ReadFILEStrings(filename2);
  result2 = measure.GetCounter();
  std::tcout << _T("Reading strings <FILE> : ") << result2 << std::endl;
  std::tcout << _T("Size of file 2         : ") << secondfile.GetLength() << std::endl;

  if (firstfile.Compare(secondfile))
  {
    std::tcout << _T("ALARM3: Read different file content!") << std::endl;
    TestDifference(firstfile,secondfile);
  }
  // Reading strings in STL IFSTREAM format
  CString thirdfile;
  measure.Reset();
  measure.Start();
  thirdfile = ReadIFSTREAMStrings(filename2);
  double result3 = measure.GetCounter();
  std::tcout << _T("Reading strings stream : ") << result3 << std::endl;
  std::tcout << _T("Size of file 2         : ") << thirdfile.GetLength() << std::endl;


  // Reading strings in UTF-16 (LE) format
  measure.Reset();
  measure.Start();
  firstfile = ReadWinFile(filename3);
  result1 = measure.GetCounter();
  std::tcout << _T("Reading strings UTF-16 : ") << result1 << std::endl;
  std::tcout << _T("Size of file 3         : ") << firstfile.GetLength() << std::endl;

  // Reading strings in UTF-16 (BE) format
  measure.Reset();
  measure.Start();
  firstfile = ReadWinFile(filename4);
  result1 = measure.GetCounter();
  std::tcout << _T("Reading Mac-OS UTF-16  : ") << result1 << std::endl;
  std::tcout << _T("Size of file 4         : ") << firstfile.GetLength() << std::endl;
}

//////////////////////////////////////////////////////////////////////////
//
// TEST RANDOM ACCESS READ-WRITE 
//
//////////////////////////////////////////////////////////////////////////

#define RECORD_SIZE 20

void 
WriteRandom(CString p_filename, int p_records)
{
  uchar buffer[100];

  WinFile file(p_filename);
  if(file.Open(winfile_write,attrib_normal))
  {
    for(int index = 0;index < p_records;++index)
    {
      sprintf_s((char*)buffer,100,"RECORD_%4.4d_NUMBER.\n",index);
      file.Puts(buffer);
    }
    file.Close();
    size_t size = file.GetFileSize();
    std::tcout << _T("Random file written: ") << p_filename.GetString() << _T(". Size: ") << size << std::endl;
  }
}

void 
ReadRandom(CString p_filename,int p_records,int p_interval)
{
  uchar buffer[100];
  uchar expect[100];
  bool  error = false;

  WinFile file(p_filename);
  if(file.Open(winfile_read | open_random_access, attrib_normal))
  {
    for(int index = 0;index < p_records;index += p_interval)
    {
      size_t newpos = (size_t)index * RECORD_SIZE;
      file.Position(FSeek::file_begin, newpos);

      sprintf_s((char*)expect,100,"RECORD_%4.4d_NUMBER.\n",index);
      file.Gets(buffer,100);
      if(strcmp((char*)buffer, (char*)expect))
      {
        error = true;
        std::tcout << _T("Error at position: ") << index << std::endl;
      }
    }
    file.Close();
    std::tcout << _T("Random file read in: ") << (error ? _T("ERRORS") : _T("OK")) << std::endl;
  }
}

void 
ReWriteRandom(CString p_filename,int p_records,int p_interval)
{
  uchar buffer[100];
  uchar expect[100];
  bool  error = false;

  WinFile file(p_filename);
  if(file.Open(open_allways | open_write | open_read | open_random_access,attrib_normal))
  {
    for(int index = 0;index < p_records;index += p_interval)
    {
      size_t newpos = (size_t)index * RECORD_SIZE;
      file.Position(FSeek::file_begin, newpos);

      sprintf_s((char*)expect, 100, "RECORD_%4.4d_NUMBER.\n", index);
      file.Gets(buffer,100);
      if (strcmp((char*)buffer, (char*)expect))
      {
        error = true;
        std::tcout << _T("Error at position: ") << index << std::endl;
      }

      sprintf_s((char*)buffer,100,"XXXXXX_%4.4d_XXXXXX:\n",index);
      file.Puts(buffer);
    }
    file.Close();
    size_t size = file.GetFileSize();
    std::tcout << _T("Random file read in: ") << (error ? _T("ERRORS") : _T("OK")) << std::endl;
    std::tcout << _T("Random file written: ") << p_filename.GetString() << _T(". Size: ") << size << std::endl;
  }
}

void 
ReadReWrittenRandom(CString p_filename,int p_records,int p_interval)
{
  uchar buffer[100];
  uchar expect[100];
  bool  error = false;

  WinFile file(p_filename);
  if(file.Open(winfile_read,attrib_normal))
  {
    for(int index = 0;index < p_records;++index)
    {
      file.Gets(buffer,100);

      if(index % p_interval == 1)
      {
        sprintf_s((char*)expect, 100, "XXXXXX_%4.4d_XXXXXX:\n", index - 1);
      }
      else
      {
        sprintf_s((char*)expect, 100, "RECORD_%4.4d_NUMBER.\n", index);
      }
      if(strcmp((char*)buffer, (char*)expect))
      {
        error = true;
        std::tcout << _T("Error at position: ") << index << std::endl;
      }
    }
    file.Close();
    std::tcout << _T("Random file read in: ") << (error ? _T("ERRORS") : _T("OK")) << std::endl;
  }
}

void TestRandomAccess()
{
  std::tcout << std::endl;
  std::tcout << _T("TESTING RANDOM ACCESS") << std::endl;

  CString filename(_T("C:\\TMP\\RandomFile.bin"));
  int n = 10000;
  int x =   300;
  int y =    15;

  // Create test file with n records
  WriteRandom(filename,n);

  // Read every n record in the file and test
  ReadRandom(filename,n,x);

  // ReWrite every y record in the file and close
  ReWriteRandom(filename,n,y);

  // ReRead every y re-written record in the file
  ReadReWrittenRandom(filename,n,y);
}

//////////////////////////////////////////////////////////////////////////
//
// TEMPORARY FILES
//
//////////////////////////////////////////////////////////////////////////

void TestTemporaryFile()
{
  std::tcout << _T("TESTING TEMPORARY FILE") << std::endl;

  WinFile file;
  file.CreateTempFileName(_T("WOC"));
  std::tcout << _T("Filename is: ") << file.GetFilename().GetString() << std::endl;
  file.DeleteToTrashcan();
}

//////////////////////////////////////////////////////////////////////////
//
// FILE TIMES
//
//////////////////////////////////////////////////////////////////////////

void TestGetFiletimes()
{
  WinFile file;
  file.CreateTempFileName(_T("ABC"));
  if(file.Open(winfile_read))
  {
    time_t created = WinFile::ConvertFileTimeToTimet(file.GetFileTimeCreated());
    tm now;
    localtime_s(&now,&created);
    char buffer[100];
    strftime(buffer,100,"",&now);

    std::tcout << _T("File: [") << file.GetFilename().GetString() << _T("] created: ") << buffer << std::endl;
//               << now.tm_year + 1900 << "-"
//               << now.tm_mon  + 1    << "-"
//               << now.tm_mday        << " "
//               << now.tm_hour        << ":"
//               << now.tm_min         << ":"
//               << now.tm_sec         << " "
//               << (now.tm_isdst ? "summertime " : "")
//               << std::endl;

    file.Close();
  }
  file.DeleteFile();
}

void TestSpecialFolder()
{
  WinFile file;
  file.SetFilenameInFolder(CSIDL_MYMUSIC,_T("MyTestFile.tmp"));
  file.Create();
  std::tcout << _T("Desktop file: ") << file.GetFilename().GetString() << std::endl;
}

//////////////////////////////////////////////////////////////////////////
//
// UTF-8
//
//////////////////////////////////////////////////////////////////////////

// Testing the Unicode translations
void TestUnicode()
{
  std::vector<CString> file_ansi;
  std::vector<CString> file_utf8;
  std::vector<CString> file_le_utf16;
  std::vector<CString> file_be_utf16;

  std::tcout << std::endl;
  std::tcout << _T("Testing 4 cases (ANSI / UTF-8 / LE-UTF-16 / BE-UTF-16") << std::endl;

  // READING ALL TEXTFILES
  WinFile file1(_T("..\\Testfiles\\testfile.txt"));
  if(file1.Open(winfile_read | FFlag::open_trans_text))
  {
    CString content;
    while(file1.Read(content))
    {
      ATLTRACE("STRING length: %d\n",content.GetLength());
      ATLTRACE("Bare strlen  : %d\n",_tcslen(content.GetString()));

      file_ansi.push_back(content);
    }
    file1.Close();
  }
  else
  {
    std::tcout << _T("ANSI FILE NOT FOUND: ") << file1.GetFilename().GetString() << std::endl;
  }

  WinFile file2(_T("..\\Testfiles\\testfile_utf8.txt"));
  if(file2.Open(winfile_read | FFlag::open_trans_text))
  {
    CString content;
    while(file2.Read(content))
    {
      file_utf8.push_back(content);
    }
    file2.Close();
  }
  else
  {
    std::tcout << _T("UTF-8 FILE NOT FOUND: ") << file2.GetFilename().GetString() << std::endl;
  }

  WinFile file3(_T("..\\Testfiles\\testfile_le_utf16.txt"));
  if(file3.Open(winfile_read | FFlag::open_trans_text))
  {
    CString content;
    while(file3.Read(content))
    {
      file_le_utf16.push_back(content);
    }
    file3.Close();
  }
  else
  {
    std::tcout << _T("LE-UTF-16 FILE NOT FOUND: ") << file3.GetFilename().GetString() << std::endl;
  }

  WinFile file4(_T("..\\Testfiles\\testfile_be_utf16.txt"));
  if(file4.Open(winfile_read | FFlag::open_trans_text))
  {
    CString content;
    while(file4.Read(content))
    {
      file_be_utf16.push_back(content);
    }
    file4.Close();
  }
  else
  {
    std::tcout << _T("BE-UTF-16 FILE NOT FOUND: ") << file4.GetFilename().GetString() << std::endl;
  }

  std::tcout << _T("All testfiles read") << std::endl;

  // WRITING ALL TEXTFILES
  file1.SetFilename(_T("..\\Testfiles\\Write_testfile.txt"));
  if(file1.Open(winfile_write | FFlag::open_trans_text))
  {
    for(auto& str : file_ansi)
    {
      if(!file1.Write(str))
      {
        std::tcout << _T("ERROR WRITING ANSI FILE: ") << file1.GetFilename().GetString() << std::endl;
      }
    }
    file1.Close();
  }
  else
  {
    std::tcout << _T("ANSI FILE NOT WRITTEN: ") << file1.GetFilename().GetString() << std::endl;
  }

  file2.SetFilename(_T("..\\Testfiles\\Write_testfile_utf8.txt"));
  file2.SetEncoding(Encoding::UTF8);
  if(file2.Open(winfile_write | FFlag::open_trans_text))
  {
    for(auto& str : file_utf8)
    {
      if(!file2.Write(str))
      {
        std::tcout << _T("ERROR WRITING UTF-8 FILE: ") << file2.GetFilename().GetString() << std::endl;
      }
    }
    file2.Close();
  }
  else
  {
    std::tcout << _T("UTF-8 FILE NOT WRITTEN: ") << file2.GetFilename().GetString() << std::endl;
  }

  file3.SetFilename(_T("..\\Testfiles\\Write_testfile_le_utf16.txt"));
  file3.SetEncoding(Encoding::LE_UTF16);
  if(file3.Open(winfile_write | FFlag::open_trans_text))
  {
    for(auto& str : file_le_utf16)
    {
      if(!file3.Write(str))
      {
        std::tcout << _T("ERROR WRITING LE-UTF-16 FILE: ") << file3.GetFilename().GetString() << std::endl;
      }
    } 
    file3.Close();
  }
  else
  {
    std::tcout << _T("LE-UTF-16 FILE NOT WRITTEN: ") << file3.GetFilename().GetString() << std::endl;
  }

  file4.SetFilename(_T("..\\Testfiles\\Write_testfile_be_utf16.txt"));
  file4.SetEncoding(Encoding::BE_UTF16);
  if(file4.Open(winfile_write | FFlag::open_trans_text))
  {
    for(auto& str : file_be_utf16)
    {
      if(!file4.Write(str))
      {
        std::tcout << _T("ERROR WRITING BE-UTF-16 FILE: ") << file4.GetFilename().GetString() << std::endl;
      }
    }
    file4.Close();
  }
  else
  {
    std::tcout << _T("BE-UTF-16 FILE NOT WRITTEN: ") << file4.GetFilename().GetString() << std::endl;
  }

  std::tcout << _T("All testfiles written") << std::endl;
  std::tcout << std::endl;
}

//////////////////////////////////////////////////////////////////////////
//
// Valid directory/file names
//
//////////////////////////////////////////////////////////////////////////

void TestValidNames()
{
  WinFile file;

  CString directory1(_T("domain\\user.name"));
  CString expected1(_T("domain_user.name"));
  CString expected2(_T("domain_user_name"));

  CString test1 = file.LegalDirectoryName(directory1);
  if(test1.Compare(expected1) != 0)
  {
    std::tcout << _T("Legal directory name check failed: ") << test1 << std::endl;
  }
  CString test2 = file.LegalDirectoryName(directory1,false);
  if(test2.Compare(expected2) != 0)
  {
    std::tcout << _T("Legal directory name check including extension failed: ") << test2 << std::endl;
  }
}

//////////////////////////////////////////////////////////////////////////
//
// Valid directory/file names
//
//////////////////////////////////////////////////////////////////////////

void TestStreamingMode()
{
  std::tcout << _T("TESTING STREAMING TO/FROM WinFile") << std::endl;

  WinFile file;
  file.CreateTempFileName(_T("STR"));
  std::tcout << _T("Filename is: ") << file.GetFilename().GetString() << std::endl;

  file.Open(winfile_write | open_trans_binary);
  if(!file.GetIsOpen())
  {
    std::tcout << _T("ALARM: File could not be opened!") << std::endl;
    return;
  }
  __int64 total1 = 0;
  __int64 total2 = 0;
  int num;
  
  for(int x = 1;x <= 100000; ++x)
  {
    total1 += x;
    file << x;
    if(file.GetLastError())
    {
      break;
    }
  }
  file.Close();

  file.Open(winfile_read | open_trans_binary);
  if(file.GetIsOpen())
  {
    for(int x = 1;x <= 100000; ++x)
    {
      file >> num;
      total2 += num;
      if(file.GetLastError())
      {
        break;
      }
    }
    file.Close();
  }
  if(total1 != total2)
  {
    std::tcout << _T("ALARM: File Not read as written!") << std::endl;
    return;
  }
  else
  {
    std::tcout << _T("Stream mode ok. Check sum: ") << total1 << std::endl;
  }
  file.DeleteToTrashcan();
  std::tcout << std::endl;
}

//////////////////////////////////////////////////////////////////////////
//
// MAIN DRIVER
//
//////////////////////////////////////////////////////////////////////////

int main()
{
  std::tcout << _T("Hello World, we are testing WinFile\n");
  std::tcout << _T("===================================\n");

  // Testing valid names
  TestValidNames();
  // Basic string write/re-read test
  TestReadingWriting();
  // Now do a performance test
  TestPerformance();
  // Test random-access read-write
  TestRandomAccess();
  // Test creating a temporary file
  TestTemporaryFile();
  // Testing filetime function
  TestGetFiletimes();
  // Testing special folders
  TestSpecialFolder();
  // Testing the Unicode translations
  TestUnicode();

  // Test WinFile streaming mode
  TestStreamingMode();

  // Wait for user approval
  std::tcout << std::endl;
  std::tcout << _T("Ready testing: ");

  _getch();

  std::tcout << _T("OK") << std::endl;
}
