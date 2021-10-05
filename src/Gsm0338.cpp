//************************************************************************************************************************
// Gsm0338.cpp
// Version 1.0 June, 2017
// Author Gerald Guiony
//************************************************************************************************************************


#include "Gsm0338.h"


//========================================================================================================================
// The standard encoding for GSM messages is the 7-bit default alphabet as defined in the 23.038 recommendation
// https://www.csoft.co.uk/support/character-sets
// https://en.wikipedia.org/wiki/GSM_03.38#GSM_7_bit_default_alphabet_and_extension_table_of_3GPP_TS_23.038_.2F_GSM_03.38
//========================================================================================================================
String Gsm0338 :: gsm0338ToAscii (const String & strGsm0338) {

	String strAscii;

	for (int i=0; i<strGsm0338.length(); i++) {

		// Note that the second part of the table is only accessible if the GSM device supports the 7-bit extension mechanism,
		// using the ESC character prefix. Otherwise, the ESC code itself is interpreted as a space, and the following
		// character will be treated as if there was no leading ESC code.

		if (strGsm0338[i] == 0x1B) { 								// Escape character

			switch (strGsm0338[i+1]) {

//				case 0x65:
//					strAscii.concat ('€');
//					i++;
//					break;

				case 0x0A:
					strAscii.concat (0x0C);
					i++;
					break;

				case 0x3C:
					strAscii.concat ('[');
					i++;
					break;

				case 0x2F:
					strAscii.concat ('\\');
					i++;
					break;

				case 0x3E:
					strAscii.concat (']');
					i++;
					break;

				case 0x14:
					strAscii.concat ('^');
					i++;
					break;

				case 0x28:
					strAscii.concat ('{');
					i++;
					break;

				case 0x40:
					strAscii.concat ('|');
					i++;
					break;

				case 0x29:
					strAscii.concat ('}');
					i++;
					break;

				case 0x3D:
					strAscii.concat ('~');
					i++;
					break;

				default:
					strAscii.concat (' ');
			}

		}
		else {
			strAscii.concat(strGsm0338[i]);
		}
	}

	return strAscii;
}


//========================================================================================================================
// The standard encoding for GSM messages is the 7-bit default alphabet as defined in the 23.038 recommendation
// https://www.csoft.co.uk/support/character-sets
// https://en.wikipedia.org/wiki/GSM_03.38#GSM_7_bit_default_alphabet_and_extension_table_of_3GPP_TS_23.038_.2F_GSM_03.38
//========================================================================================================================
String Gsm0338 :: asciiToGsm0338 (const String & strAscii) {

	String strGsm0338;

	for (int i=0; i<strAscii.length(); i++) {

		switch (strAscii [i]) {

//			case '€':
//				strGsm0338.concat (0x1B);
//				strGsm0338.concat (0x65);
//				break;

			case 0x0C:
				strGsm0338.concat (0x1B);
				strGsm0338.concat (0x0A);
				break;

			case '[':
				strGsm0338.concat (0x1B);
				strGsm0338.concat (0x3C);
				break;

			case '\\':
				strGsm0338.concat (0x1B);
				strGsm0338.concat (0x2F);
				break;

			case ']':
				strGsm0338.concat (0x1B);
				strGsm0338.concat (0x3E);
				break;

			case '^':
				strGsm0338.concat (0x1B);
				strGsm0338.concat (0x14);
				break;

			case '{':
				strGsm0338.concat (0x1B);
				strGsm0338.concat (0x28);
				break;

			case '|':
				strGsm0338.concat (0x1B);
				strGsm0338.concat (0x40);
				break;

			case '}':
				strGsm0338.concat (0x1B);
				strGsm0338.concat (0x29);
				break;

			case '~':
				strGsm0338.concat (0x1B);
				strGsm0338.concat (0x3D);
				break;

			default:
				strGsm0338.concat (strAscii[i]);
		}
	}

	return strGsm0338;
}
