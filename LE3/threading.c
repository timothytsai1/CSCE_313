#include <threading.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>
#include <stdio.h>

// ucontext_t main_context;

void t_init()
{
        // Initialize all contexts to INVALID
        for (int i = 0; i < NUM_CTX; i++)
        {
                contexts[i].state = INVALID;
        }
        current_context_idx = 0;
        getcontext(&contexts[0].context);
        contexts[0].state = VALID;

}

int32_t t_create(fptr foo, int32_t arg1, int32_t arg2)
{
        // printf("[DEBUG] Context Created \n");

        // Find the first invalid context and create a new one
        for (uint8_t idx = 0; idx < NUM_CTX; idx++)
        {

                if (contexts[idx].state == INVALID)
                {
                        // Mark the context as VALID and set the current index
                        getcontext(&contexts[idx].context);
                        // current_context_idx = idx;
                        contexts[idx].state = VALID;
                        
                        contexts[idx].context.uc_stack.ss_sp = (char *)malloc(STK_SZ);
                        // if (contexts[idx].context.uc_stack.ss_sp == NULL)
                        // {
                        //         return -1;
                        // }

                        contexts[idx].context.uc_stack.ss_size = STK_SZ;
                        //     contexts[idx].context.uc_link = &main_context;
                        contexts[idx].context.uc_link = &contexts[current_context_idx].context;
                        contexts[idx].context.uc_stack.ss_flags = 0;

                        // printf("[DBG] Created context at %d for (%d -> %d)\n", idx, arg1, arg2);
                        makecontext(&contexts[idx].context, (ctx_ptr)foo, 2, arg1, arg2);

                        return 0;
                }
        }
        return 1;
}

int32_t t_yield()
{

        getcontext(&contexts[current_context_idx].context);
        // Save the current context and find another valid one to switch to
        int remaining_context = 0;
        for (uint8_t idx = 1; idx < NUM_CTX; idx++)
        {
                uint8_t id_next = (current_context_idx + idx) % NUM_CTX;
                if (contexts[id_next].state == VALID)
                {
                        // printf("[DEBUG] Context switch: From context %d to context %d\n", current_context_idx, idx);
                        uint8_t old_context_idx = current_context_idx;
                        current_context_idx = id_next;
                        // printf("[DBG] Switching contexts from %d to %d\n", old_context_idx, current_context_idx);
                        swapcontext(&contexts[old_context_idx].context, &contexts[id_next].context);
                        // printf("[DBG] Swapcontext returned %d\n", retval);
                        remaining_context++;
                        break;
                }
        }

        // Count remaining contexts
        
        // for (int i = 0; i < NUM_CTX; i++)
        // {
        //         if (contexts[i].state == VALID)
        //         {
        //                 remaining_context++;
        //         }
        // }
        // printf("[DEBUG] Number of context remaining %d\n", remaining_context);

        // return remaining_context - 1;
        return remaining_context;
}

void t_finish()
{
        contexts[current_context_idx].state = DONE;
        free(contexts[current_context_idx].context.uc_stack.ss_sp);
        memset(&contexts[current_context_idx], 0, sizeof(struct worker_context));
        t_yield();
}
