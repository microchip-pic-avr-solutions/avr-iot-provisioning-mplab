// The MCC generated m2m_fwinfo.h file is missing a couple of includes which makes it impossible to mock.
// To avoid having to modify the MCC generated file the current file replaces the original one
// since ceedling will start by looking in the test/support folder for header files. This will adds the missing includes below.
#include <stdbool.h>
#include "m2m_types.h"
// Then the original MCC generated file is included so that it can be mocked
#include "../../mcc_generated_files/winc/m2m/m2m_fwinfo.h"