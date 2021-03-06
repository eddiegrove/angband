/*
 * File: src/object/obj-flag.c
 * Purpose: functions to deal with object flags
 *
 * Copyright (c) 2011 Chris Carr
 *
 * This work is free software; you can redistribute it and/or modify it
 * under the terms of either:
 *
 * a) the GNU General Public License as published by the Free Software
 *    Foundation, version 2, or
 *
 * b) the "Angband licence":
 *    This software may be copied and distributed for educational, research,
 *    and not for profit purposes provided that this copyright and statement
 *    are included in all such copies.  Other copyrights may also apply.
 */
#include "angband.h"

/**
 * Details of the different object flags in the game.
 * See src/object/obj-flag.h for structure
 */
const struct object_flag object_flag_table[] =
{
    #define OF(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s) \
            { OF_##a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s },
    #include "list-object-flags.h"
    #undef OF
};

/**
 * Object flag names
 */
static const char *flag_names[] =
{
    #define OF(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s) #a,
    #include "list-object-flags.h"
    #undef OF
    ""
};

/**
 * Create a "mask" of flags of a specific type or ID threshold.
 *
 * \param f is the flag array we're filling
 * \param id is whether we're masking by ID level
 * \param ... is the list of flags or ID types we're looking for
 *
 * N.B. OFT_MAX must be the last item in the ... list
 */
void create_mask(bitflag f[OF_SIZE], bool id, ...)
{
	const struct object_flag *of_ptr;
	int i;
	va_list args;

	/* a dummy first flag is assumed below, so make sure it is still there */
	assert(OF_NONE == 0);

	/* using OFT_MAX as end marker, but OFID are a separate enum */
	assert (OFID_MAX <= OFT_MAX);

	of_wipe(f);

	va_start(args, id);

	/* Process each type in the va_args */
    for (i = va_arg(args, int); i != OFT_MAX; i = va_arg(args, int)) {
		for (of_ptr = object_flag_table; of_ptr->index < OF_MAX; of_ptr++)
			if ((id && of_ptr->id == i) || (!id && of_ptr->type == i)) {
				/* flag code crashes badly with too small flags */
				if (of_ptr->index >= FLAG_START)
					of_on(f, of_ptr->index);
			}
	}

	va_end(args);

	return;
}

/**
 * Create the mask of unlearnable flags.
 */
void of_unlearnable_mask(bitflag unlearnable[OF_SIZE])
{
	create_mask(unlearnable, TRUE, OFID_NONE, OFT_MAX);
}

/**
 * Create the mask of curse flags.
 */
void of_curse_mask(bitflag curse_mask[OF_SIZE])
{
	create_mask(curse_mask, FALSE, OFT_CURSE, OFT_MAX);
}

/**
 * Create the mask of pval flags.
 */
void of_pval_mask(bitflag pval_mask[OF_SIZE])
{
	create_mask(pval_mask, FALSE, OFT_PVAL, OFT_STAT, OFT_MAX);
}


/**
 * Print a message when an object flag is identified by use.
 *
 * \param flag is the flag being noticed
 * \param name is the object name 
 */
void flag_message(int flag, char *name)
{
	const struct object_flag *of_ptr = &object_flag_table[flag];

	if (!streq(of_ptr->message, ""))
		msg(of_ptr->message, name);

	return;
}

/**
 * Determine whether a flagset includes any curse flags.
 */
bool cursed_p(bitflag *f)
{
	bitflag f2[OF_SIZE];

	of_wipe(f2);
	create_mask(f2, FALSE, OFT_CURSE, OFT_MAX);

	return of_is_inter(f, f2);
}

/**
 * Determine whether a timed flag or its permanent equivalent are set.
 *
 * \param flag is the TMD_ flag for which we are checking.
 */
bool check_state(int flag)
{
	const struct object_flag *of_ptr;

	/* If the TMD_ flag is set, we return immediately */
	if (p_ptr->timed[flag])
		return TRUE;

	/* If not, we look for an OF_ equivalent and test for it */
	for (of_ptr = object_flag_table; of_ptr->index < OF_MAX; of_ptr++)
		if (of_ptr->timed == flag && p_ptr->state.flags[of_ptr->index])
			return TRUE;

	return FALSE;
}

/**
 * Log the names of a flagset to a file.
 *
 * \param f is the set of flags we are logging.
 * \param log_file is the file to which we are logging the names.
 */
void log_flags(bitflag *f, ang_file *log_file)
{
	int i;

	file_putf(log_file, "Object flags are:\n");
	for (i = 0; i < OF_MAX; i++)
		if (of_has(f, i))
			file_putf(log_file, "%s\n", flag_names[i]);
}

/**
 * Log the name of a flag to a file.
 *
 * \param flag is the flag to log.
 * \param log_file is ... oh come on how obvious does it need to be?
 */
const char *flag_name(int flag)
{
	return flag_names[flag];
}

/**
 * Get the slot multiplier for a flag's power rating
 *
 * \param flag is the flag in question.
 * \param slot is the wield_slot it's in.
 */
s16b slot_mult(int flag, int slot)
{
	const struct object_flag *of_ptr = &object_flag_table[flag];

	switch (slot) {
		case INVEN_WIELD: 	return of_ptr->weapon;
		case INVEN_BOW:		return of_ptr->bow;
		case INVEN_LEFT:
		case INVEN_RIGHT:	return of_ptr->ring;
		case INVEN_NECK:	return of_ptr->amulet;
		case INVEN_LIGHT:	return of_ptr->light;
		case INVEN_BODY:	return of_ptr->body;
		case INVEN_OUTER:	return of_ptr->cloak;
		case INVEN_ARM:		return of_ptr->shield;
		case INVEN_HEAD:	return of_ptr->hat;
		case INVEN_HANDS:	return of_ptr->gloves;
		case INVEN_FEET:	return of_ptr->boots;
		default: 			return 1;
	}
}

/**
 * Return the base power rating for a flag.
 */
s32b flag_power(int flag)
{
	const struct object_flag *of_ptr = &object_flag_table[flag];

	return of_ptr->power;
}

/**
 * Ascertain whether a flag is granular (pval-based) or binary.
 */
bool flag_uses_pval(int flag)
{
	const struct object_flag *of_ptr = &object_flag_table[flag];

	return of_ptr->pval;
}

/**
 * Return the OFT_ type of a flag.
 */
int obj_flag_type(int flag)
{
	const struct object_flag *of_ptr = &object_flag_table[flag];

	return of_ptr->type;
}

/**
 * Return the pval weighting of a flag. (Some pvals are more important than
 * others.)
 */
int pval_mult(int flag)
{
	const struct object_flag *of_ptr = &object_flag_table[flag];

	return of_ptr->pval_mult;
}
