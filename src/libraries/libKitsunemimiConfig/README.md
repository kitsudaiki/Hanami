# libKitsunemimiConfig

## Description

This library provides a simple interface for reading config files.


## Usage by example

Content of example config-file:

```
[DEFAULT]
string_val = asdf.asdf
int_val = 2
float_val = 123.0
string_list = a,b,c
bool_value = true
```

Example usage in code:

```cpp
#include <libKitsunemimiConfig/config_handler.h>
#include <libKitsunemimiCommon/logger.h>

ErrorContainer error;

// init configuration
// called anywhere at the beginning of the programm
Kitsunemimi::initConfig(m_testFilePath, error), true);

// register values
REGISTER_STRING_CONFIG("DEFAULT", "string_val", error, "");
REGISTER_INT_CONFIG("DEFAULT", "int_val", error, 42);
REGISTER_INT_CONFIG("DEFAULT", "another_int_val", error, 42);

// check if config is valid and all registrations were successfull
bool valid = Kitsunemimi::isConfigValid();
// is true in this example
if(valid == false) {
	LOG_ERROR(error);
}

// all register options:
//
// REGISTER_STRING_CONFIG
// REGISTER_INT_CONFIG
// REGISTER_FLOAT_CONFIG
// REGISTER_BOOL_CONFIG
// REGISTER_STRING_ARRAY_CONFIG

bool success = false;

std::string firstValue = GET_STRING_CONFIG("DEFAULT", "string_val", success);
//     variable success is true
long number1 = GET_INT_CONFIG("DEFAULT", "int_val", success);
//     variable success is true
long number2 = GET_INT_CONFIG("DEFAULT", "another_int_val", success);
//     variable success is true

// all get options:
//
// GET_STRING_CONFIG
// GET_INT_CONFIG
// GET_FLOAT_CONFIG
// GET_BOOL_CONFIG
// GET_STRING_ARRAY_CONFIG

// get on not registered value
std::string fail = GET_STRING_CONFIG("DEFAULT", "fail", success);
//     variable success is false
```