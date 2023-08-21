@echo BEWARE!!
@echo -
@echo Compares in UNICODE build must yield NO differences
@echo Compares in MBCS build will yield differences for japanese characters
@echo -
@echo Comparing intput/output files
fc /b testfile.txt          write_testfile.txt
fc /b testfile_utf8.txt     write_testfile_utf8.txt
fc /b testfile_le_utf16.txt write_testfile_le_utf16.txt
fc /b testfile_be_utf16.txt write_testfile_be_utf16.txt
