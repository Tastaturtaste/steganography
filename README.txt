How to use:
1. 	For the python version:
		a) Install the required packages with: 'pip install -r requirements.txt'
		b) Use the script as specified below
	For the c++ version:
		a) Make sure vcpkg is installed (https://github.com/microsoft/vcpkg)
		b) Make sure vcpkg has user-wide integration enabled (./vcpkg integrate install)
		c) Build Steganography.sln in Visual Studio
		d) Use the executable generated in /Steganography/bin/ as specified below
2a. 	To encode some arbitrary file in some png-file: 'executable' 'name_of_carrier_pic'.png --file 'name_of_file'.'file_ending'
2b. 	To encode some string in some png-file: 	'executable' 'name_of_carrier_pic'.png --text "SomeStringYouWantToEncode"'
3. 	To decode something from png-file: 		'executable' 'name_of_carrier_pic'.png

Trying to encode something which does not fit in the carrier_pic results in a failure.
Trying to decode something from a picture which has nothing encoded results in a failure.