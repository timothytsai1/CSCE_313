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
}

int32_t t_create(fptr foo, int32_t arg1, int32_t arg2)
{
        // Find the first invalid context and create a new one
        for (uint8_t i = current_context_idx; i < NUM_CTX; i++)
        {
                if (contexts[i].state == INVALID)
                {
                        // Mark the context as VALID and set the current index
                        contexts[i].state = VALID;
                        current_context_idx = i;
                        
                        contexts[current_context_idx].context.uc_stack.ss_sp = malloc(STK_SZ);
                        if (contexts[current_context_idx].context.uc_stack.ss_sp == NULL)
                        {
                                return -1;
                        }

                        contexts[current_context_idx].context.uc_stack.ss_size = STK_SZ;
                        //     contexts[current_context_idx].context.uc_link = &main_context;
                        contexts[current_context_idx].context.uc_link = NULL;
                        contexts[current_context_idx].context.uc_stack.ss_flags = 0;

                        getcontext(&contexts[current_context_idx].context);
                        makecontext(&contexts[current_context_idx].context, (ctx_ptr)foo, 2, arg1, arg2);

                        return 0;
                }
        }
        return -1;
}

int32_t t_yield()
{
        // Save the current context and find another valid one to switch to
        if (getcontext(&contexts[current_context_idx].context) == 0)
        {
                for (uint8_t i = 0; i < NUM_CTX; i++)
                {
                        if (i != current_context_idx && contexts[i].state == VALID)
                        {
                                uint8_t old_context_idx = current_context_idx;
                                current_context_idx = i;
                                swapcontext(&contexts[old_context_idx].context, &contexts[current_context_idx].context);
                                break;
                        }
                }
        }

        // Count remaining contexts
        int remaining_context = 0;
        for (int i = 0; i < NUM_CTX; i++)
        {
                if (contexts[i].state == VALID)
                {
                        remaining_context++;
                }
        }

        return remaining_context - 1; // Number of tasks left
}

void t_finish()
{
        if (contexts[current_context_idx].context.uc_stack.ss_sp != NULL)
        {
                free(contexts[current_context_idx].context.uc_stack.ss_sp);
                contexts[current_context_idx].context.uc_stack.ss_sp = NULL;
        }

        memset(&contexts[current_context_idx], 0, sizeof(struct worker_context));
        //     contexts[current_context_idx].state = DONE;

        t_yield();
}
