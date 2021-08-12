
#include "octa.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

const porp_info_t g_prop_info_table[PROP_TYPE_COUNT] = {
	[PTI_FLAGS] = {.name = "flags", .size = sizeof(flags_t)},
	[PTI_POS] = {.name = "pos", .size = sizeof(pos_t)},
	[PTI_COLOR] = {.name = "color", .size = sizeof(color_t)},
};

octa_t g_octa = {0};

void pti_print(pti_t pti)
{
	assert(pti < PROP_TYPE_COUNT);
	fputs(g_prop_info_table[pti].name, stdout);
}

void ptis_add(ptis_t* ptis, pti_t pti)
{
	if (ptis->len == 0)
	{
		ptis->arr = malloc(sizeof(pti_t));
		ptis->arr[0] = pti;
		ptis->len = 1;
		return;
	}
	unsigned int new_len = ptis->len + 1;
	ptis->arr = realloc(ptis->arr, new_len * sizeof(pti_t));
	/* As arr should always be sorted, the good spot is searched for from the
	 * right and ptis greater than the new one are shifted to the right. */
	unsigned int i = ptis->len - 1;
	while (1)
	{
		if (ptis->arr[i] < pti)
		{
			ptis->arr[i+1] = pti;
			break;
		}
		else if (i == 0) /* && ptis->arr[i] >= pti */
		{
			assert(ptis->arr[i] != pti);
			ptis->arr[i+1] = ptis->arr[i];
			ptis->arr[i] = pti;
			break;
		}
		else
		{
			assert(ptis->arr[i] != pti);
			ptis->arr[i+1] = ptis->arr[i];
			i--;
		}
	}
	ptis->len = new_len;
}

void ptis_copy(const ptis_t* src, ptis_t* dst)
{
	dst->arr = malloc(src->len * sizeof(pti_t));
	memcpy(dst->arr, src->arr, src->len * sizeof(pti_t));
	dst->len = src->len;
}

int pti_eq(const ptis_t* a, const ptis_t* b)
{
	if (a->len != b->len)
	{
		return 0;
	}
	for (unsigned int i = 0; i < a->len; i++)
	{
		if (a->arr[i] != b->arr[i])
		{
			return 0;
		}
	}
	return 1;
}

void ptis_print(const ptis_t* ptis)
{
	fputs("ptis", stdout);
	fputs("[", stdout);
	for (unsigned int i = 0; i < ptis->len; i++)
	{
		pti_print(ptis->arr[i]);
		if (i != ptis->len - 1)
		{
			fputs(",", stdout);
		}
	}
	fputs("]", stdout);
}

void colt_init(colt_t* colt, const ptis_t* ptis)
{
	colt->row_count = 0;
	ptis_copy(ptis, &colt->ptis);
	colt->col_data_arr = calloc(ptis->len, sizeof(void*));
}

void colt_lengthen(colt_t* colt, unsigned int by_how_much)
{
	unsigned int new_row_count = colt->row_count + by_how_much;
	for (unsigned int i = 0; i < colt->ptis.len; i++)
	{
		unsigned int prop_size = g_prop_info_table[colt->ptis.arr[i]].size;
		colt->col_data_arr[i] = realloc(colt->col_data_arr[i],
			new_row_count * prop_size);
	}
	assert(colt->ptis.arr[0] == PTI_FLAGS);
	for (unsigned int i = colt->row_count; i < new_row_count; i++)
	{
		((flags_t*)colt->col_data_arr[0])[i].bit_set.exists = 0;
	}
	colt->row_count = new_row_count;
}

int colt_does_obj_exist(const colt_t* colt, unsigned int row_index)
{
	assert(colt->ptis.arr[0] == PTI_FLAGS);
	return ((flags_t*)colt->col_data_arr[0])[row_index].bit_set.exists;
}

void colt_print(const colt_t* colt)
{
	fputs("colt", stdout);
	fputs("(", stdout);
	printf("row_count:%d", colt->row_count);
	fputs(",", stdout);
	ptis_print(&colt->ptis);
	fputs(",", stdout);
	assert(colt->ptis.arr[0] == PTI_FLAGS);
	fputs("[", stdout);
	for (unsigned int i = 0; i < colt->row_count; i++)
	{
		fputs(colt_does_obj_exist(colt, i) ? "!" : "_", stdout);
	}
	fputs("]", stdout);
	fputs(")", stdout);
}

unsigned int octa_add_colt(const ptis_t* ptis)
{
	g_octa.len++;
	g_octa.colt_arr = realloc(g_octa.colt_arr, g_octa.len * sizeof(colt_t));
	unsigned int index = g_octa.len - 1;
	colt_init(&g_octa.colt_arr[index], ptis);
	return index;
}

oi_t octa_alloc_obj(const ptis_t* ptis)
{
	colt_t* colt = NULL;
	unsigned int colt_index;
	for (unsigned int i = 0; i < g_octa.len; i++)
	{
		if (pti_eq(&g_octa.colt_arr[i].ptis, ptis))
		{
			colt = &g_octa.colt_arr[i];
			colt_index = i;
			break;
		}
	}
	if (colt == NULL)
	{
		unsigned int i = octa_add_colt(ptis);
		colt = &g_octa.colt_arr[i];
		colt_index = i;
	}
	for (unsigned int i = 0; i < colt->row_count; i++)
	{
		if (!colt_does_obj_exist(colt, i))
		{
			return (oi_t){.colt_index = colt_index, .row_index = i};
		}
	}
	unsigned int old_row_count = colt->row_count;
	colt_lengthen(colt, colt->row_count / 2 + 4);
	return (oi_t){.colt_index = colt_index, .row_index = old_row_count};
}

void* octa_get_obj_prop(oi_t oi, pti_t pti)
{
	assert(oi.colt_index < g_octa.len);
	colt_t* colt = &g_octa.colt_arr[oi.colt_index];
	assert(oi.row_index < colt->row_count);
	unsigned int i;
	for (i = 0; i < colt->ptis.len; i++)
	{
		if (colt->ptis.arr[i] == pti)
		{
			break;
		}
	}
	assert(i < colt->ptis.len);
	void* col_data = colt->col_data_arr[i];
	unsigned int prop_size = g_prop_info_table[colt->ptis.arr[i]].size;
	return (char*)col_data + oi.row_index * prop_size;
}

void octa_print(void)
{
	fputs("octa", stdout);
	fputs("(", stdout);
	printf("len:%d", g_octa.len);
	fputs(",", stdout);
	fputs("[", stdout);
	for (unsigned int i = 0; i < g_octa.len; i++)
	{
		colt_print(&g_octa.colt_arr[i]);
		if (i != g_octa.len - 1)
		{
			fputs(",", stdout);
		}
	}
	fputs("]", stdout);
	fputs(")", stdout);
}