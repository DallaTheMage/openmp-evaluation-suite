#include "data/writers/writer.h"
#include "data/writers/csv.h"

ResultWriter *create_writer(void) {
    #if FILETYPE == FILETYPE_CSV
        return create_csv_writer();
    #else
        #error SELECTED FILETYPE IS NOT SUPPORTED YET.
    #endif
}

void destroy_writer(ResultWriter *writer) {
    if (!writer) return;

    #if FILETYPE == FILETYPE_CSV
        destroy_csv_writer(writer);
    #else
        #error SELECTED FILETYPE IS NOT SUPPORTED YET.
    #endif
}
