#include "data/readers/reader.h"
#include "data/readers/csv.h"

ResultReader *create_reader(void) {
    #if FILETYPE == FILETYPE_CSV
        return create_csv_reader();
    #else
        #error SELECTED FILETYPE IS NOT SUPPORTED YET.
    #endif
}
