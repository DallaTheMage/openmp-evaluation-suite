#include "data/writers/writer.h"
#include "data/writers/csv.h"

ResultWriter *create_writer(void) {
    #if FILETYPE == FILETYPE_CSV
        return create_csv_writer();
    #else
        #error SELECTED FILETYPE IS NOT SUPPORTED YET.
    #endif
}
