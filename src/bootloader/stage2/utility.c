#include "utility.h"

/**
 * Align a number to a multiple of another number.
 *
 * This function takes a number and ensures that it is a multiple of
 * another number. If the number is not a multiple of the alignment
 * number, it is rounded up to the next multiple of the alignment
 * number.
 *
 * @param number The number to align.
 * @param alignTo The number that the number should be aligned to.
 *
 * @return The aligned number.
 * I think i dont need it anymore but "if it aint broke, don't fix it"
 */
uint32_t align(uint32_t number, uint32_t alignTo)
{
    if (alignTo == 0)
        return number;

    uint32_t rem = number % alignTo;
    return (rem > 0) ? (number + alignTo - rem) : number;
}
