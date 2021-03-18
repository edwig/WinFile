// WinFile.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "WinFile.h"
#include "HPFCounter.h"
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <conio.h>
#include <time.h>
#include <shlobj.h>
#include <atlstr.h>

using std::string;
using std::ifstream;

//////////////////////////////////////////////////////////////////////////
//
// BASIC WRITE AND RE-READ TEST in text translation mode
//
//////////////////////////////////////////////////////////////////////////

void TestReadingWriting()
{
  std::cout << "Writing the output file\n";
  std::cout << "=======================\n";

//WinFile file;
  WinFile file("C:\\TMP\\Testfile.txt");
// file.SetFilenameByDialog(nullptr,true,"Test file","txt","Tempfile.txt");
//  file.CreateTempFileName("TMF","txt");

  string testing("This is a test for WinFile\n");

  if(file.Open(winfile_write | FFlag::open_trans_text))
  {

    for(auto number = 0; number < 10; ++number)
    {
      file.Write(testing);
    }
    file.Close();
  }
  std::cout << "Written." << std::endl;

  // Now read it back
  std::cout << "Reading back the file\n";
  std::cout << "=====================\n";

  if (file.Open(winfile_read | FFlag::open_trans_text))
  {
    string line;
    while(file.Read(line))
    {
      if(strcmp(line.c_str(), testing.c_str()))
      {
        std::cout << "ERROR reading testfile back" << std::endl;
      }
    }
    file.Close();
  }
  std::cout << "Done reading." << std::endl;
  std::cout.flush();


  if(file.CanAccess(true))
  {
    std::cout << "The file [" << file.GetFilename() << "] exists and is writable" << std::endl;
  }
  file.DeleteToTrashcan();
  if(!file.Exists())
  {
    std::cout << "The file [" << file.GetFilename() << "] is gone" << std::endl;
  }
}

//////////////////////////////////////////////////////////////////////////
//
// PERFORMANCE TEST TO COMPARE WITH fgets()
//
//////////////////////////////////////////////////////////////////////////

string ReadWinFile(string p_filename)
{
  string result;
  string line;

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

string ReadFILE(string p_filename)
{
  string result;

  FILE* file = nullptr;
  fopen_s(&file, p_filename.c_str(), "rt");
  if (file)
  {
    int ch;
    while ((ch = fgetc(file)) != EOF)
    {
      result += ch;
    }
    fclose(file);
  }
  return result;
}

string ReadWinFileStrings(string p_filename)
{
  string result;
  unsigned char buffer[1024];

  WinFile file(p_filename);
  if (file.Open(winfile_read | FFlag::open_trans_text))
  {
    while (file.Gets(buffer,1024))
    {
      result += (char*)buffer;
    }
    file.Close();
  }
  return result;
}

string ReadFILEStrings(string p_filename)
{
  string result;
  char buffer[1024];

  FILE* file = nullptr;
  fopen_s(&file, p_filename.c_str(), "rt");
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

string ReadIFSTREAMStrings(string p_filename)
{
  string result;
  string line;

  ifstream file(p_filename);
  if (file.is_open())
  {
    while(std::getline(file,line))
    {
      result += line + '\n';
    }
    file.close();
  }
  return result;
}

void TestPerformance()
{
  HPFCounter measure;
  string filename = "C:\\BIN\\KEYDB.cfg";
//string filename = "E:\\TMP\\KEYDB.cfg";

  std::cout << "TESTING READ PERFORMANCE OF STRINGS" << std::endl;

  measure.Reset();
  measure.Start();
  string firstfile = ReadWinFile(filename);
  double result1 = measure.GetCounter();
  std::cout << "Reading WinFile        : " << result1 << std::endl;
  std::cout << "Size of file 1         : " << firstfile.size() << std::endl;

  measure.Reset();
  measure.Start();
  string secondfile = ReadFILE(filename);
  double result2 = measure.GetCounter();
  std::cout << "Reading <FILE>         : " << result2 << std::endl;
  std::cout << "Size of file 2         : " << secondfile.size() << std::endl;

  if(strcmp(firstfile.c_str(), secondfile.c_str()))
  {
    std::cout << "ALARM1: Read different file content!" << std::endl;
  }

  measure.Reset();
  measure.Start();
  CString secondfile2(ReadFILE(filename).c_str());
  result2 = measure.GetCounter();
  std::cout << "Reading <FILE>         : " << result2 << std::endl;
  std::cout << "Size of file 2 in ATL  : " << secondfile2.GetLength() << std::endl;

  if (strcmp(firstfile.c_str(), secondfile.c_str()))
  {
    std::cout << "ALARM1: Read different file content!" << std::endl;
  }

  // Reading strings in our WinFile format
  measure.Reset();
  measure.Start();
  firstfile = ReadWinFileStrings(filename);
  result1 = measure.GetCounter();
  std::cout << "Reading strings WinFile: " << result1 << std::endl;
  std::cout << "Size of file 1         : " << firstfile.size() << std::endl;
  // Reading strings in standard <FILE> format
  measure.Reset();
  measure.Start();
  secondfile = ReadFILEStrings(filename);
  result2 = measure.GetCounter();
  std::cout << "Reading strings <FILE> : " << result2 << std::endl;
  std::cout << "Size of file 2         : " << secondfile.size() << std::endl;

  if (strcmp(firstfile.c_str(), secondfile.c_str()))
  {
    std::cout << "ALARM2: Read different file content!" << std::endl;
  }
  // Reading strings in STL IFSTREAM format
  string thirdfile;
  measure.Reset();
  measure.Start();
  thirdfile = ReadIFSTREAMStrings(filename);
  double result3 = measure.GetCounter();
  std::cout << "Reading strings stream : " << result3 << std::endl;
  std::cout << "Size of file 3         : " << thirdfile.size() << std::endl;

  if (strcmp(firstfile.c_str(), thirdfile.c_str()))
  {
    std::cout << "ALARM3: Read different file content!" << std::endl;
  }
}

//////////////////////////////////////////////////////////////////////////
//
// TEST RANDOM ACCESS READ-WRITE 
//
//////////////////////////////////////////////////////////////////////////

#define RECORD_SIZE 20

void WriteRandom(string p_filename, int p_records)
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
    std::cout << "Random file written: " << p_filename << ". Size: " << size << std::endl;
  }
}

void ReadRandom(string p_filename,int p_records,int p_interval)
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
        std::cout << "Error at position: " << index << std::endl;
      }
    }
    file.Close();
    std::cout << "Random file read in: " << (error ? "ERRORS" : "OK") << std::endl;
  }
}

void ReWriteRandom(string p_filename,int p_records,int p_interval)
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
        std::cout << "Error at position: " << index << std::endl;
      }

      sprintf_s((char*)buffer,100,"XXXXXX_%4.4d_XXXXXX:\n",index);
      file.Puts(buffer);
    }
    file.Close();
    size_t size = file.GetFileSize();
    std::cout << "Random file read in: " << (error ? "ERRORS" : "OK") << std::endl;
    std::cout << "Random file written: " << p_filename << ". Size: " << size << std::endl;
  }
}

void ReadReWrittenRandom(string p_filename,int p_records,int p_interval)
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
        std::cout << "Error at position: " << index << std::endl;
      }
    }
    file.Close();
    std::cout << "Random file read in: " << (error ? "ERRORS" : "OK") << std::endl;
  }
}

void TestRandomAccess()
{
  std::cout << "TESTING RANDOM ACCESS" << std::endl;

  string filename("C:\\TMP\\RandomFile.bin");
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
  std::cout << "TESTING TEMPORARY FILE" << std::endl;

  WinFile file;
  file.CreateTempFileName("WOC");
  std::cout << "Filename is: " << file.GetFilename() << std::endl;
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
  file.CreateTempFileName("ABC");
  if(file.Open(winfile_read))
  {
    time_t created = WinFile::ConvertFileTimeToTimet(file.GetFileTimeCreated());
    tm now;
    localtime_s(&now,&created);
    char buffer[100];
    strftime(buffer,100,"",&now);

    std::cout << "File: [" << file.GetFilename() << "] created: " << buffer << std::endl;
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
  file.SetFilenameInFolder(CSIDL_MYMUSIC,"MyTestFile.tmp");
  file.Create();
  std::cout << "Desktop file: " << file.GetFilename() << std::endl;
}

// Testing the UTF-8 check
void TestUnicodeUTF8()
{
  WinFile file("C:\\TMP\\test_utf8.txt");
  if(file.Open(winfile_read))
  {
    std::string content;
    file.Read(content);
    bool is8 = file.IsTextUnicodeUTF8((const uchar*)content.c_str(),content.size());

    std::cout << "Testfile UTF8 open" << std::endl;
    std::cout << "Check for UTF8: " << (is8 ? "YES" : "NO") << std::endl;

    file.Close();
  }
  else
  {
    std::cout << "UTF8 FILE NOT FOUND: " << file.GetFilename() << std::endl;
  }
}

//////////////////////////////////////////////////////////////////////////
//
// MAIN DRIVER
//
//////////////////////////////////////////////////////////////////////////

int main()
{
  std::cout << "Hello World, we are testing WinFile\n";
  std::cout << "===================================\n";

  HRESULT hr = CoInitialize(nullptr);

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
  // Testing the UTF-8 check
  TestUnicodeUTF8();

  // Wait for user approval
  std::cout << std::endl;
  std::cout << "Ready testing: ";
  int throwaway = _getch();
  std::cout << "OK" << std::endl;

  CoUninitialize();
}
