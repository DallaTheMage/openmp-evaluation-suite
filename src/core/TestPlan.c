#include <stdlib.h>
#include <assert.h>

#include "core/TestPlan.h"


/* -------------------------------------------------------------------------- */
/* INTERNAL HELPERS                                                           */
/* -------------------------------------------------------------------------- */

/**
 * Popola un TestSet con tutte le configurazioni:
 *
 *     num_threads × chunk_size
 *
 * Se explicit_thread_filter > 0, vengono inserite esclusivamente
 * le configurazioni relative a quel numero di thread.
 */
static bool populate_test_points(
    TestPointArray *points,
    const Configuration *config,
    uint32_t explicit_thread_filter
)
{
    assert(points != NULL);
    assert(config != NULL);

    points->data = NULL;
    points->count = 0;


    const size_t thread_count =
        (explicit_thread_filter > 0)
            ? 1
            : config->execution.threads.count;

    const size_t chunk_count =
        config->execution.chunk_sizes.count;


    if (thread_count == 0 || chunk_count == 0) {
        return true;
    }


    const size_t capacity =
        thread_count * chunk_count;


    points->data =
        calloc(capacity, sizeof(TestPoint));

    if (!points->data) {
        return false;
    }


    size_t index = 0;


    for (size_t t = 0;
         t < config->execution.threads.count;
         ++t) {

        const uint32_t num_threads =
            config->execution.threads.data[t];


        if (explicit_thread_filter > 0 &&
            num_threads != explicit_thread_filter) {

            continue;
        }


        for (size_t c = 0;
             c < chunk_count;
             ++c) {

            TestPoint *point =
                &points->data[index];


            /*
             * TestPointUID è locale al TestSet.
             *
             * La chiave completa del test sarà:
             *
             *     TestSetUID + TestPointUID
             */
            point->id =
                (TestPointUID)index;


            point->num_threads =
                num_threads;

            point->chunk_size =
                config->execution.chunk_sizes.data[c];


            ++index;
        }
    }


    /*
     * Il filtro esplicito potrebbe non aver trovato alcun thread.
     */
    if (index == 0) {

        free(points->data);

        points->data = NULL;
        points->count = 0;

        return true;
    }


    points->count = index;

    return true;
}


/* -------------------------------------------------------------------------- */

/**
 * Distrugge il contenuto di un TestSet.
 *
 * La DataView è non-owning.
 */
static void destroy_test_set(
    TestSet *test_set
)
{
    if (!test_set) {
        return;
    }


    free(test_set->points.data);

    test_set->points.data = NULL;
    test_set->points.count = 0;
}


/* -------------------------------------------------------------------------- */

/**
 * Distrugge tutti i TestSet contenuti nel piano.
 */
static void destroy_test_sets(
    TestSet *sets,
    size_t count
)
{
    if (!sets) {
        return;
    }


    for (size_t i = 0;
         i < count;
         ++i) {

        destroy_test_set(&sets[i]);
    }


    free(sets);
}


/* -------------------------------------------------------------------------- */
/* PROPORTIONAL TEST PLAN                                                     */
/* -------------------------------------------------------------------------- */

TestPlan *generate_proportional_test_plan(
    DataBuffer *buffer,
    const Configuration *config
)
{
    assert(buffer != NULL);
    assert(config != NULL);


    const uint64_t max_allowed_elements =
        config->derived.double_count;


    /*
     * Ogni numero di thread determina:
     *
     *     window_size =
     *         size_per_thread × num_threads
     *
     * Verifichiamo tutto prima di effettuare allocazioni.
     */
    for (size_t t = 0;
         t < config->execution.threads.count;
         ++t) {

        const uint32_t num_threads =
            config->execution.threads.data[t];

        const uint64_t window_size =
            config->derived.size_per_thread * num_threads;


        if (window_size > max_allowed_elements) {
            return NULL;
        }
    }


    TestPlan *plan =
        calloc(1, sizeof(TestPlan));

    if (!plan) {
        return NULL;
    }


    const size_t thread_count =
        config->execution.threads.count;


    plan->count =
        thread_count * VIEW_TYPE_COUNT;


    if (plan->count == 0) {
        return plan;
    }


    plan->data =
        calloc(plan->count, sizeof(TestSet));

    if (!plan->data) {

        free(plan);

        return NULL;
    }


    TestSetUID next_test_set_id = 0;


    for (size_t t = 0;
         t < thread_count;
         ++t) {

        const uint32_t num_threads =
            config->execution.threads.data[t];


        const uint64_t window_size =
            config->derived.size_per_thread * num_threads;


        for (int type = 0;
             type < VIEW_TYPE_COUNT;
             ++type) {

            TestSet *test_set =
                &plan->data[next_test_set_id];


            test_set->id =
                next_test_set_id++;


            test_set->view =
                create_data_view(
                    buffer,
                    (ViewType)type,
                    &config->shapes[type],
                    0,
                    window_size
                );


            if (!populate_test_points(
                    &test_set->points,
                    config,
                    num_threads)) {

                destroy_test_plan(plan);

                return NULL;
            }
        }
    }


    return plan;
}


/* -------------------------------------------------------------------------- */
/* FULL SCALE TEST PLAN                                                       */
/* -------------------------------------------------------------------------- */

TestPlan *generate_full_scale_test_plan(
    DataBuffer *buffer,
    const Configuration *config
)
{
    assert(buffer != NULL);
    assert(config != NULL);


    const uint64_t max_allowed_elements =
        config->derived.double_count;


    /*
     * Verifica preventiva delle window prima di qualsiasi allocazione.
     */
    for (size_t p = 0;
         p < config->execution.window_sizes.count;
         ++p) {

        const uint64_t window_size =
            config->execution.window_sizes.data[p];


        if (window_size > max_allowed_elements) {
            return NULL;
        }
    }


    TestPlan *plan =
        calloc(1, sizeof(TestPlan));

    if (!plan) {
        return NULL;
    }


    const size_t window_count =
        config->execution.window_sizes.count;


    plan->count =
        window_count * VIEW_TYPE_COUNT;


    if (plan->count == 0) {
        return plan;
    }


    plan->data =
        calloc(plan->count, sizeof(TestSet));

    if (!plan->data) {

        free(plan);

        return NULL;
    }


    TestSetUID next_test_set_id = 0;


    for (size_t p = 0;
         p < window_count;
         ++p) {

        const uint64_t window_size =
            config->execution.window_sizes.data[p];


        for (int type = 0;
             type < VIEW_TYPE_COUNT;
             ++type) {

            TestSet *test_set =
                &plan->data[next_test_set_id];


            test_set->id =
                next_test_set_id++;


            test_set->view =
                create_data_view(
                    buffer,
                    (ViewType)type,
                    &config->shapes[type],
                    0,
                    window_size
                );


            if (!populate_test_points(
                    &test_set->points,
                    config,
                    0)) {

                destroy_test_plan(plan);

                return NULL;
            }
        }
    }


    return plan;
}


/* -------------------------------------------------------------------------- */
/* VALIDATION                                                                 */
/* -------------------------------------------------------------------------- */

bool validate_test_plan(
    const TestPlan *plan
)
{
    if (!plan) {
        return false;
    }


    for (size_t i = 0;
         i < plan->count;
         ++i) {

        const TestSet *test_set =
            &plan->data[i];


        /*
         * TestSetUID deve corrispondere alla posizione
         * all'interno del piano.
         */
        if (test_set->id != (TestSetUID)i) {
            return false;
        }


        /*
         * Ogni DataView deve avere un buffer associato.
         */
        if (!test_set->view.buffer) {
            return false;
        }


        /*
         * Verifica i TestPoint.
         */
        for (size_t p = 0;
             p < test_set->points.count;
             ++p) {

            const TestPoint *point =
                &test_set->points.data[p];


            /*
             * TestPointUID è locale al TestSet.
             */
            if (point->id != (TestPointUID)p) {
                return false;
            }


            if (point->num_threads == 0 ||
                point->chunk_size == 0) {

                return false;
            }
        }
    }


    return true;
}


/* -------------------------------------------------------------------------- */
/* DESTRUCTION                                                               */
/* -------------------------------------------------------------------------- */

void destroy_test_plan(
    TestPlan *plan
)
{
    if (!plan) {
        return;
    }


    destroy_test_sets(
        plan->data,
        plan->count
    );


    free(plan);
}
