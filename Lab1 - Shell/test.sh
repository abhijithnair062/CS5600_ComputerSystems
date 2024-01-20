## Few pointers on adding new test points
# Make sure all temp directories and files are deleted after every test if created any
# Follow the numbering on the error messages
# Make sure failed = "yes" for negative cases

#TODO(ABHIJITH): Add delete if exists condition for directories and files in a seperate function which is called before each test
failed=''

#Mother of all Tests
final_check(){
  if [ "$failed" = "" ] ; then
    echo ALL TESTS PASSED
  else
    echo FAILED
  fi
}
#################################################### UNIT TESTS BELOW #################################################
# Test exit - positive
./shell56 <<EOF
exit 5
EOF
rv=$?
if [ $rv == 5 ] ; then
  echo '1.Passed for exit 5'
else
  echo '1.Failed for exit 5'
  failed="yes"
fi

# Test exit - positive
./shell56 <<EOF
exit
EOF
rv=$?
if [ $rv == 0 ] ; then
  echo '2.Passed for exit'
else
  echo '2.Failed for exit'
  failed="yes"
fi

# Test exit - positive
./shell56 <<EOF
exit -100
EOF
rv=$?
if [ $rv == 0 ] ; then
  echo '3.Passed for exit -100'
else
  echo '3.Failed for exit -100'
  failed="yes"
fi

# Test for empty line
output="$(./shell56 <<EOF
EOF
)"
if [ -z $output] ; then
  echo '4.Passed for empty input'
else
  echo '4.Failed for empty input'
  failed="yes"
fi

# Test for multiple empty lines
output="$(./shell56 <<EOF


EOF
)"
if [ -z $output] ; then
  echo '5.Passed for multiple empty lines'
else
  echo '5.Failed for multiple empty lines'
  failed="yes"
fi

#Test for cd with #1 valid argument
touch temp
mkdir tempF
./shell56>temp <<"EOF"
cd tempF
echo $?
EOF
if [ $(cat temp) = "0" ] ; then
  echo '6.Passed for cd with 1 valid argument'
else
  echo '6.Failed for cd with 1 valid argument'
  failed="yes"
fi
rm temp
rmdir tempF
#Test for cd with no argument
touch temp #Create a temporary file
./shell56>temp <<"EOF"
cd
echo $?
EOF
if [ $(cat temp) = "0" ] ; then
  echo '7.Passed for cd with no argument'
else
  echo '7.Failed for cd with no argument'
  failed="yes"
fi
rm temp #Remove the temporary file


#Test for cd with invalid argument
touch temp
mkdir tempF
./shell56>temp <<"EOF"
cd tempF 2
echo $?
EOF
if [ $(cat temp) = "1" ] ; then
  echo '8.Passed for cd with invalid argument'
else
  echo '8.Failed for cd with invalid argument'
  failed="yes"
fi
rm temp #Remove the temporary file
rmdir tempF


#Test for cd with invalid path
touch temp #Create a temporary file
./shell56>temp <<"EOF"
cd tempF
echo $?
EOF
if [ $(cat temp) = "1" ] ; then
  echo '9.Passed for cd with invalid path'
else
  echo '9.Failed for cd with invalid path'
  failed="yes"
fi
rm temp
#TODO(KARTHIK): Add tests for pwd similar to cd

#Test for external command with $?
touch temp
./shell56>temp <<"EOF"
false
echo $?
EOF
if [ $(cat temp) = "1" ] ; then
  echo '10. Passed for external command'
else
  echo '10. Failed for external command'
  failed="yes"
fi
rm temp

#TODO(KARTHIK): Add tests for external commands similar to the one above

#Test for pipe 1
touch temp
./shell56>temp <<"EOF"
echo "Hello World" | tr '[:lower:]' '[:upper:]'
EOF
if [ "$(cat temp)" = "HELLO WORLD" ] ; then
  echo '11. Passed for pipe 1'
else
  echo '11. Failed for pipe 1'
  failed="yes"
fi
rm temp

#Test for pipe 2
touch temp
./shell56>temp <<"EOF"
cat /etc/passwd | cut -d: -f1 | sort
EOF
if cat temp | grep -q "daemon" ; then
  echo '12. Passed for pipe 2'
else
  echo '12. Failed for pipe 2'
  failed="yes"
fi
rm temp

#Test for pipe 3
touch temp
./shell56>temp <<"EOF"
echo "The quick brown fox" | tr '[:space:]' '\n' | sort | uniq -c
EOF
if cat temp | grep -q "1 brown" ; then
  echo '13. Passed for pipe 3'
else
  echo '13. Failed for pipe 3'
  failed="yes"
fi
rm temp

#Test for pipe 4
touch temp
./shell56>temp <<"EOF"
echo "apple banana cherry date" | tr ' ' '\n' | sort | uniq -c | sort -n
EOF
if cat temp | grep -q "1 apple" ; then
  echo '14. Passed for pipe 4'
else
  echo '14. Failed for pipe 4'
  failed="yes"
fi
rm temp

#Test for pipe 6
touch temp
./shell56>temp <<"EOF"
echo "apple banana cherry date elderberry fig" | tr ' ' '\n' | sort | uniq -c | sort -n | tail -n 2 | sed 's/ *[0-9]* //'
EOF
if cat temp | grep -q "fig" ; then
  echo '15. Passed for pipe 6'
else
  echo '15. Failed for pipe 6'
  failed="yes"
fi
rm temp

#Test for pipe 7
touch temp
./shell56>temp <<"EOF"
echo "apple banana cherry date elderberry fig grape" | tr ' ' '\n' | sort | uniq -c | sort -n | tail -n 3 | sed 's/ *[0-9]* //' | tr '[:lower:]' '[:upper:]'
EOF
if cat temp | grep -q "GRAPE" ; then
  echo '16. Passed for pipe 7'
else
  echo '16. Failed for pipe 7'
  failed="yes"
fi
rm temp

#Test for pipe 8
touch temp
./shell56>temp <<"EOF"
echo "apple banana cherry date elderberry fig grape honeydew" | tr ' ' '\n' | sort | uniq -c | sort -n | tail -n 4 | sed 's/ *[0-9]* //' | tr '[:lower:]' '[:upper:]' | grep -v "D"
EOF
if cat temp | grep -q "GRAPE" ; then
  echo '17. Passed for pipe 8'
else
  echo '17. Failed for pipe 8'
  failed="yes"
fi
rm temp

#Test for invalid pipe
touch temp
./shell56>temp <<"EOF"
|
echo $?
EOF
if [ "$(cat temp)" = "1" ] || [ "$(cat temp)" = "0" ] ; then
  echo '18. Passed for invalid pipe command 1'
else
  echo '18. Failed for invalid pipe command 1'
  failed="yes"
fi
rm temp

#Test for invalid pipe
touch temp
./shell56>temp <<"EOF"
| |
echo $?
EOF
if [ "$(cat temp)" = "1" ] || [ "$(cat temp)" = "0" ] ; then
  echo '19. Passed for invalid pipe command 2'
else
  echo '19. Failed for invalid pipe command 2'
  failed="yes"
fi
rm temp

#Test for invalid pipe
touch temp
./shell56>temp <<"EOF"
| | echo
echo $?
EOF
if [ "$(cat temp)" = "1" ] || [ "$(cat temp)" = "0" ] ; then
  echo '20. Passed for invalid pipe command 3'
else
  echo '20. Failed for invalid pipe command 3'
  failed="yes"
fi
rm temp

#Test for invalid pipe
touch temp
./shell56>temp <<"EOF"
echo |
echo $?
EOF
if [ "$(cat temp)" = "1" ] || [ "$(cat temp)" = "0" ] ; then
  echo '21. Passed for invalid pipe command 4'
else
  echo '21. Failed for invalid pipe command 4'
  failed="yes"
fi
rm temp

#Test for invalid pipe
touch temp
./shell56>temp <<"EOF"
echo | |
echo $?
EOF
if [ "$(cat temp)" = "1" ] || [ "$(cat temp)" = "0" ]  ; then
  echo '22. Passed for invalid pipe command 5'
else
  echo '22. Failed for invalid pipe command 5'
  failed="yes"
fi
rm temp

#Test for invalid pipe
touch temp
./shell56>temp <<"EOF"
echo | | false
echo $?
EOF
if [ "$(cat temp)" = "1" ] || [ "$(cat temp)" = "0" ]  ; then
  echo '23. Passed for invalid pipe command 6'
else
  echo '23. Failed for invalid pipe command 6'
  failed="yes"
fi
rm temp

#Test for invalid pipeline stages > 8
touch temp
./shell56>temp <<"EOF"
echo "apple banana cherry date elderberry fig grape honeydew" | tr ' ' '\n' | sort | uniq -c | sort -n | tail -n 4 | sed 's/ *[0-9]* //' | tr '[:lower:]' '[:upper:]' | grep -v "D" | echo foo
echo $?
EOF
if [ "$(cat temp)" = "1" ] || [ "$(cat temp)" = "0" ]  ; then
  echo '24. Passed for more than 8 pipeline stages'
else
  echo '24. Failed for more than 8 pipeline stages'
  failed="yes"
fi
rm temp



#Test for invalid commands
touch temp
./shell56>temp <<"EOF"
ls >> cat
echo $?
EOF
if [ $(cat temp) = "1" ] ; then
  echo '27. Passed for invalid commands 1'
else
  echo '27. Failed for invalid commands 1'
  failed="yes"
fi
rm temp

#Test for invalid commands
touch temp
./shell56>temp <<"EOF"
ls <<< cat
echo $?
EOF
if [ $(cat temp) = "1" ] ; then
  echo '28. Passed for invalid commands 2'
else
  echo '28. Failed for invalid commands 2'
  failed="yes"
fi
rm temp

#Test for invalid commands
touch temp
./shell56>temp <<"EOF"
ls > | cat
echo $?
EOF
if [ $(cat temp) = "1" ] ; then
  echo '29. Passed for invalid commands 3'
else
  echo '29. Failed for invalid commands 3'
  failed="yes"
fi
rm temp

#Test for invalid commands
touch temp
./shell56>temp <<"EOF"
ls | < echo foo | cat
echo $?
EOF
if [ $(cat temp) = "1" ] ; then
  echo '30. Passed for invalid commands 4'
else
  echo '30. Failed for invalid commands 4'
  failed="yes"
fi
rm temp

#Test for invalid commands
touch temp
./shell56>temp <<"EOF"
echo foo < | | false
echo $?
EOF
if [ $(cat temp) = "1" ] ; then
  echo '31. Passed for invalid commands 5'
else
  echo '31. Failed for invalid commands 5'
  failed="yes"
fi
rm temp

#Test for invalid commands
touch temp
./shell56>temp <<"EOF"
false >< true | echo foo | cat  < true
echo $?
EOF
if [ $(cat temp) = "1" ] ; then
  echo '32. Passed for invalid commands 6'
else
  echo '32. Failed for invalid commands 6'
  failed="yes"
fi
rm temp

#Test for invalid commands
touch temp
./shell56>temp <<"EOF"
true > < | false > | true
echo $?
EOF
if [ $(cat temp) = "1" ] ; then
  echo '34. Passed for invalid commands 7'
else
  echo '34. Failed for invalid commands 7'
  failed="yes"
fi
rm temp

#Test for invalid commands
touch temp
./shell56>temp <<"EOF"
< true > < | false > < | true >
echo $?
EOF
if [ $(cat temp) = "1" ] ; then
  echo '35. Passed for invalid commands 8'
else
  echo '35. Failed for invalid commands 8'
  failed="yes"
fi
rm temp

#Simple Test
touch temp
./shell56>temp <<"EOF"
cd
echo $?
EOF
if [ $(cat temp) = "0" ] ; then
  echo '36. Passed for single valid command'
else
  echo '36. Failed for single valid command'
  failed="yes"
fi
rm temp

#Step 6 testing
./shell56 <<EOF
ls -l > output
EOF
if cat output | grep -q "test.sh" ; then
  echo '37. Passed for single file redirection >'
else
  echo '37. Failed for single file redirection >'
  failed="yes"
fi
rm output


#Step 6 testing
ls > output
./shell56>temp <<EOF
cat < output
EOF
if cat temp | grep -q "test.sh" ; then
  echo '38. Passed for single file redirection <'
else
  echo '38. Failed for single file redirection <'
  failed="yes"
fi
rm temp
rm output
#Step 6 testing with pipe
./shell56 <<EOF
ls | cat | cat > temp
EOF
if cat temp | grep -q "test.sh" ; then
  echo '39. Passed for single file redirection with pipe'
else
  echo '39. Failed for single file redirection with pipe'
  failed="yes"
fi
rm temp

#Step 6 testing with pipe
./shell56 <<EOF
ls > first > second
EOF
if cat first | grep -q "test.sh" ; then
  echo '40. Passed for multiple redirection'
else
  echo '40. Failed for multiple redirection'
  failed="yes"
fi
rm first

#Step 6 testing with pipe
./shell56 <<EOF
ls > first > second
EOF
if cat first | grep -q "test.sh" ; then
  echo '41. Passed for multiple redirection'
else
  echo '41. Failed for multiple redirection'
  failed="yes"
fi
rm first

#Step 6 testing with pipe
touch temp
./shell56>temp <<EOF
ls > /dev/null | cat
EOF
if cat temp | grep -q "test.sh" ; then
  echo '42. Passed for abnormal redirection'
else
  echo '42. Failed for abnormal redirection'
  failed="yes"
fi
rm temp

#Step 6 testing with pipe
touch temp
./shell56>temp <<EOF
ls | cat < /dev/null
EOF
if cat temp | grep -q "test.sh" ; then
  echo '43. Passed for abnormal redirection 2'
else
  echo '43. Failed for abnormal redirection 2'
  failed="yes"
fi
rm temp
#For running final tests
final_check