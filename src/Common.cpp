#include "Common.h"

// Define the globals instance in one translation unit to avoid multiple
// definition linker errors when Common.h is included by multiple .cpp files.
Globals globals;
