//************************************************************************************************************************
// Gsm0338.h
// Version 1.0 June, 2017
// Author Gerald Guiony
//************************************************************************************************************************

#pragma once

#include <WString.h>


namespace a6gsm {

//------------------------------------------------------------------------------
//
class Gsm0338
{
public:

	static String gsm0338ToAscii (const String & strGsm0338);
	static String asciiToGsm0338 (const String & strAscii);

};

}
