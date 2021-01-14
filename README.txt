How to use:
1. 	For the python version, stegano.py, install the required packages with: 'pip install -r requirements.txt'
	For the c++ version, Steganox64.exe, you don't have to do anything.
2a. 	To encode some arbitrary file in some png-file: 'python stegano.py name_of_carrier_pic.png --file name_of_file.file_ending'
2b. 	To encode some string in some png-file: 	'python stegano.py name_of_carrier_pic.png --text "SomeStringYouWantToEncode"'
3. 	To decode something from png-file: 		'python stegano.py name_of_carrier_pic.png'

Trying to encode something which does not fit in the carrier_pic results in a failure.
Trying to decode something from a picture which has nothing encoded results in a failure.