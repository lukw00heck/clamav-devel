/*
 *  Copyright (C) 2006 Michal 'GiM' Spadlinski http://gim.org.pl/
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301, USA.
 */

/*
 * lzma.c
 *
 * o2:28:18 CEST 2oo6-25-o6 		- initial 0xA4/0x536
 * oo:29:4o CEST 2oo6-26-o6 		- 0x1cd/0x536 [+0x129]
 * o2:13:19 CEST 2oo6-o1-o7, 2oo6-3o-o6 - 0x536/0x536
 *
 */

#if HAVE_CONFIG_H
#include "clamav-config.h"
#endif

#ifdef CL_EXPERIMENTAL
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "cltypes.h"
#include "pe.h"
#include "rebuildpe.h"
#include "others.h"
#include "mew.h"
#include "packlibs.h"
#include "rebuildpe.h"

#define EC32(x) le32_to_host(x) /* Convert little endian to host */
#define CE32(x) be32_to_host(x) /* Convert big endian to host */

/* modifies all parameters */
/* northfox does this shitty way,
 * this should be done with just a bswap
 */
char *lzma_bswap_4861dc(struct lzmastate *p, char *old_edx)
{
	/* dumb_dump_start
	 *

	old_edx was 'uint32_t *' before and in mew_lzma there was
	&new_edx where new_edx = var1C

	uint32_t loc_esi, loc_edi;
	uint8_t *loc_eax;

	p->p2 = loc_esi = 0;
	p->p0 = loc_eax = (uint8_t *)*old_edx;
	*old_edx = 5;
	do {
		loc_esi = p->p2 << 8;
		loc_edi = *(uint8_t *)((loc_eax)++);
		loc_esi |= loc_edi;
		(*old_edx)--;
		p->p2 = loc_esi;
	} while (*old_edx);
	p->p0 = loc_eax;
	p->p1 = 0xffffffff;

	* dumb_dump_end
	*/

	/* XXX, mine replacement */
	p->p2 = EC32(CE32(((uint32_t)cli_readint32(old_edx + 1))));
	p->p1 = 0xffffffff;
	p->p0 = old_edx + 5;

	return p->p0;
}

int lzma_486248 (struct lzmastate *p, char **old_ecx, char *src, uint32_t size)
{
	uint32_t loc_esi, loc_edi, loc_eax, loc_ecx, ret;
	if (!CLI_ISCONTAINED(src, size, *old_ecx, 4) || !CLI_ISCONTAINED(src, size, p->p0, 1))
		return -1;
	loc_esi = p->p1;
	loc_eax = loc_esi >> 0xb;
	loc_ecx = cli_readint32(*old_ecx);
	ret = loc_ecx&0xffff;
	(loc_eax) *=  ret;
	loc_edi = p->p2;
	if (loc_edi < loc_eax)
	{
		/* 48625f */
		p->p1 = loc_eax;
		loc_esi = ret;
		loc_edi = ((int32_t)(0x800 - ret) >> 5) + ((loc_eax&0xffff0000) | ret); 
								/* signed<-sar, &|<-mov ax, [ecx] */
		loc_ecx = (loc_ecx&0xffff0000)|(loc_edi&0xffff);
		cli_writeint32(*old_ecx, loc_ecx);

		ret = 0;
	} else {
		/* 48629e */
		loc_esi -= loc_eax;
		loc_edi -= loc_eax;
		p->p1 = loc_esi;
		p->p2 = loc_edi;
		loc_eax = (loc_eax & 0xffff0000) | ret;
		loc_esi = (loc_esi & 0xffff0000) | (ret >> 5);
		loc_eax -= loc_esi;

		loc_ecx = (loc_ecx&0xffff0000)|(loc_eax&0xffff);
		cli_writeint32(*old_ecx, loc_ecx);

		ret = 1;
	}
	loc_eax = p->p1;
	if (loc_eax < 0x1000000)
	{
		*old_ecx = p->p0;
		loc_edi = (*(uint8_t *)(p->p0));
		loc_esi = ((p->p2) << 8) | loc_edi;
		(*old_ecx)++;
		loc_eax <<= 8;
		p->p2 = loc_esi;
		p->p1 = loc_eax;
		p->p0 = *old_ecx;
	}
	return ret;

}

uint32_t lzma_48635C(uint8_t znaczek, char **old_ecx, struct lzmastate *p, uint32_t *retval, char *src, uint32_t size)
{
	uint32_t loc_esi = (znaczek&0xff) >> 7, /* msb */
		loc_ebx, ret;
	char *loc_edi;
	znaczek <<= 1;
	ret = loc_esi << 9;
	loc_edi = *old_ecx;
	*old_ecx = loc_edi + ret + 0x202;
	if ((ret = lzma_486248 (p, old_ecx, src, size)) == -1)
		return -1;
	loc_ebx = ret | 2;

	while (loc_esi == ret)
	{
		if (loc_ebx >= 0x100)
		{
			ret = (ret&0xffffff00) | (loc_ebx&0xff);
			*retval = ret;
			return 0;
		}
		loc_esi = (znaczek&0xff) >> 7;
		znaczek <<= 1;
		ret = ((loc_esi + 1) << 8) + loc_ebx;
		*old_ecx = loc_edi + ret*2;
		if ((ret = lzma_486248 (p, old_ecx, src, size)) == -1)
			return -1;
		loc_ebx += loc_ebx;
		loc_ebx |= ret;
	}
	loc_esi = 0x100;
	while (loc_ebx < loc_esi)
	{
		loc_ebx += loc_ebx;
		*old_ecx = loc_edi + loc_ebx;
		if ((ret = lzma_486248 (p, old_ecx, src, size)) == -1)
			return -1;
		loc_ebx |= ret;
	}
	ret = (ret&0xffffff00) | (loc_ebx&0xff);
	*retval = ret;
	return 0;
}

uint32_t lzma_4862e0 (struct lzmastate *p, char **old_ecx, uint32_t *old_edx, uint32_t *retval, char *src, uint32_t size)
{
	uint32_t loc_ebx, loc_esi, stack_ecx, ret;
	char *loc_edi;

	loc_ebx = *old_edx;
	ret = 1;
	loc_edi = *old_ecx;
	if (loc_ebx && !(loc_ebx&0x80000000))
	{
		/* loc_4862f1 */
		stack_ecx = loc_ebx;
		do {
			loc_esi = ret+ret;
			*old_ecx = loc_edi + loc_esi;
			if ((ret = lzma_486248 (p, old_ecx, src, size)) == -1)
				return -1;
			ret += loc_esi;
			stack_ecx--;
		} while (stack_ecx);
	} 
	/* loc_48630b */
	  /* unneeded
	*old_ecx = (uint8_t *)loc_ebx;
	  */
	
	*old_edx = 1 << (loc_ebx&0xff);
	ret -= *old_edx;
	*retval = ret;
	return 0;
}

/* old_edx - write only */
uint32_t lzma_4863da (uint32_t var0, struct lzmastate *p, char  **old_ecx, uint32_t *old_edx, uint32_t *retval, char *src, uint32_t size)
{
	uint32_t ret;
	char *loc_esi = *old_ecx;

	if ((ret = lzma_486248 (p, old_ecx, src, size)) == -1)
		return -1;
	if (ret)
	{
		/* loc_4863ff */
		*old_ecx = loc_esi+2;
		if ((ret = lzma_486248 (p, old_ecx, src, size)) == -1)
			return -1;
		if (ret)
		{
			/* loc_486429 */
			*old_edx = 8;
			*old_ecx = loc_esi + 0x204;
			if (lzma_4862e0 (p, old_ecx, old_edx, &ret, src, size) == -1)
				return -1;
			ret += 0x10;
		} else {
			/* loc_48640e */
			ret = var0 << 4;
			*old_edx = 3;
			*old_ecx = loc_esi + 0x104 + ret;
			if (lzma_4862e0 (p, old_ecx, old_edx, &ret, src, size) == -1)
				return -1;
			ret += 0x8;
		}
	} else {
		/* loc_4863e9 */
		ret = var0 << 4;
		*old_edx = 3;
		*old_ecx = loc_esi + 0x4 + ret;
		if (lzma_4862e0 (p, old_ecx, old_edx, &ret, src, size) == -1)
			return -1;
	}
	*retval = ret;
	return 0;
}

uint32_t lzma_486204 (struct lzmastate *p, uint32_t old_edx, uint32_t *retval, char *src, uint32_t size)
{
	uint32_t loc_esi, loc_edi, loc_ebx, loc_eax;
	char *loc_edx;
	loc_esi = p->p1;
	loc_edi = p->p2;
	loc_eax = 0;
	if (old_edx && !(old_edx&0x80000000))
	{
		/* loc_4866212 */
		loc_ebx = old_edx;
		do {
			loc_esi >>= 1;
			loc_eax <<= 1;
			if (loc_edi >= loc_esi)
			{
				loc_edi -= loc_esi;
				loc_eax |= 1;
			}
			/* loc_486222 */
			if (loc_esi < 0x1000000)
			{
				if (!CLI_ISCONTAINED(src, size, p->p0, 1))
					return -1;
				loc_edx = p->p0;
				loc_edi <<= 8;
				loc_esi <<= 8;
				loc_edi |= (*loc_edx)&0xff; /* movzx ebp, byte ptr [edx] */
				p->p0 = ++loc_edx;
			}
			loc_ebx--;
		} while (loc_ebx);

	}
	p->p2 = loc_edi;
	p->p1 = loc_esi;
	*retval = loc_eax;
	return 0;
}

uint32_t lzma_48631a (struct lzmastate *p, char **old_ecx, uint32_t *old_edx, uint32_t *retval, char *src, uint32_t size)
{
	uint32_t copy1, copy2;
	uint32_t loc_esi, loc_edi, ret;
	char *loc_ebx;

	copy1 = *old_edx;
	loc_edi = 0;
	loc_ebx = *old_ecx;
	*old_edx = 1;
	copy2 = (uint32_t)loc_edi;

	if (copy1 <= (uint32_t)loc_edi)
	{
		*retval = copy2;
		return 0;
	}

	do {
		loc_esi = *old_edx + *old_edx;
		*old_ecx = loc_esi + loc_ebx;
		if ((ret = lzma_486248 (p, old_ecx, src, size)) == -1)
			return -1;
		/* unneeded *old_ecx  = loc_edi; */
		*old_edx = loc_esi + ret;
		/* ret <<= (uint32_t)(*old_ecx)&0xff; */
		ret <<= (loc_edi&0xff);
		copy2 |= ret;
		loc_edi++;
	} while (loc_edi < copy1);

	*retval = copy2;
	return 0;
}

int mew_lzma(struct pe_image_section_hdr *section_hdr, char *orgsource, char *buf, uint32_t size_sum, uint32_t vma, uint32_t special)
{
	uint32_t var08, var0C, var10, var14, var18, var20, var24, var28, var34;
	struct lzmastate var40;
	uint32_t new_eax, new_edx, temp, loc_edi, loc_esi;
	int i, mainloop;
	char var1, var30;
	char *source = buf, *dest, *new_ebx, *new_ecx, *var0C_ecxcopy, *var2C;
	char *pushed_esi = NULL, *pushed_ebx = NULL;
	uint32_t pushed_edx=0;

	if (special)
	{
		pushed_edx = cli_readint32(source);
		source += 4;
	}
	temp = cli_readint32(source) - vma;
	source += 4;
	if (!special) pushed_ebx = source;
	new_ebx = orgsource + temp;

    do {
        mainloop = 1;
	do {
		/* loc_486450 */
		if (!special)
		{
			source = pushed_ebx;
			if (cli_readint32(source) == 0)
			{
				return 0;
			}
		}
		var28 = cli_readint32 (source);
		source += 4;
		temp = cli_readint32 (source) - vma;
		var18 = (uint32_t)orgsource + temp;
		if (special) pushed_esi = orgsource + temp;
		source += 4;
		temp = cli_readint32 (source);
		source += 5; /* yes, five */
		var2C = source;
		source += temp;
		if (special) pushed_ebx = source;
		else pushed_ebx = source;
		var1 = 0;
		dest = new_ebx;
		
		if(!CLI_ISCONTAINED(orgsource, size_sum, dest, 0x6E6C))
			return -1;
		for (i=0; i<0x1b9b; i++)
		{
			cli_writeint32(dest, 0x4000400);
			dest += 4;
		}
		loc_esi = 0;
		var08 = var20 = 0;
		loc_edi = 1;
		var14 = var10 = var24 = 1;

		lzma_bswap_4861dc(&var40, var2C);
		new_edx = 0;
	} while (var28 <= loc_esi); /* source = 0 */

	cli_dbgmsg("MEWlzma: entering do while loop\n");
	do {
		/* loc_4864a5 */
		new_eax = var08 & 3;
		new_ecx = (((loc_esi << 4) + new_eax)*2) + new_ebx;
		var0C = new_eax;
		if ((new_eax = lzma_486248 (&var40, &new_ecx, orgsource, size_sum)) == -1)
			return -1;
		if (new_eax)
		{
			/* loc_486549 */
			new_ecx = new_ebx + loc_esi*2 + 0x180;
			var20 = 1;
			/* eax=1 */
			if ((new_eax = lzma_486248 (&var40, &new_ecx, orgsource, size_sum)) == -1)
				return -1;
			if (new_eax != 1)
			{
				/* loc_486627 */
				var24 = var10;
				var10 = var14;
				/* xor eax,eax; cmp esi, 7; setnl al; dec eax; add eax, 0Ah */
				/* new_eax = (((loc_esi >= 7)-1)&0xFFFFFFFD) + 0xA; */
				new_eax = loc_esi>=7 ? 10:7;
				new_ecx = new_ebx + 0x664;
				var14 = loc_edi;
				loc_esi = new_eax;
				if (lzma_4863da (var0C, &var40, &new_ecx, &new_edx, &new_eax, orgsource, size_sum) == -1)
					return -1;
				var0C = new_eax;
				if (var0C >= 4)
					new_eax = 3;

				/* loc_486662 */
				new_edx = 6;
				new_eax <<= 7;
				new_ecx = new_eax + new_ebx + 0x360;
				if (lzma_4862e0 (&var40, &new_ecx, &new_edx, &new_eax, orgsource, size_sum) == -1)
					return -1;
				if (new_eax < 4)
				{ 
					/* loc_4866ca */
					loc_edi = new_eax;
				} else {
					/* loc_48667d */
					uint32_t loc_ecx;
					loc_ecx = ((int32_t)new_eax >> 1)-1; /* sar */
					loc_edi = ((new_eax&1)|2) << (loc_ecx&0xff);
					if (new_eax >= 0xe)
					{
						/* loc_4866ab */
						new_edx = loc_ecx - 4;
						if (lzma_486204 (&var40, new_edx, &new_eax, orgsource, size_sum) == -1)
							return -1;
						loc_edi += new_eax << 4;

						new_edx = 4;
						new_ecx = new_ebx + 0x644;
					} else {
						/* loc_486691 */
						new_edx = loc_ecx;
						loc_ecx = loc_edi - new_eax;
						new_ecx =  new_ebx + loc_ecx*2 + 0x55e;
					}
					/* loc_4866a2 */
					if (lzma_48631a (&var40, &new_ecx, &new_edx, &new_eax, orgsource, size_sum) == -1)
						return -1;
					loc_edi += new_eax;
				}
				loc_edi++;
			} else {
				/* loc_486568 */
				new_ecx = new_ebx + loc_esi*2 + 0x198;
				if ((new_eax = lzma_486248 (&var40, &new_ecx, orgsource, size_sum)) == -1)
					return -1;
				if (new_eax)
				{
					/* loc_4865bd */
					new_ecx = new_ebx + loc_esi*2 + 0x1B0;
					if ((new_eax = lzma_486248 (&var40, &new_ecx, orgsource, size_sum)) == -1)
						return -1;
					if (new_eax)
					{
						/* loc_4865d2 */
						new_ecx = new_ebx + loc_esi*2 + 0x1C8;
						if ((new_eax = lzma_486248 (&var40, &new_ecx, orgsource, size_sum)) == -1)
							return -1;
						if (new_eax) {
							/* loc_4865ea */
							new_eax = var24;
							var24 = var10;
						} else {
							/* loc_4865e5 */
							new_eax = var10;
						}
						/* loc_4865f3 */
						var10 = var14;
					} else {
						/* loc_4865cd */
						new_eax = var14;
					}
					/* loc_4865f9 */
					var14 = loc_edi;
					loc_edi = new_eax;
				} else {
					/* loc_48657e */
					new_eax = ((loc_esi + 0xf) << 4) + var0C;
					new_ecx = new_ebx + new_eax*2;
					if ((new_eax = lzma_486248 (&var40, &new_ecx, orgsource, size_sum)) == -1)
						return -1;
					if (!new_eax) {
						uint32_t loc_ecx;
						/* loc_486593 */
						loc_ecx = var08;
						loc_ecx -= loc_edi;
						/* loc_esi = ((((loc_esi >= 7)-1)&0xFFFFFFFE) + 0xB); */
						loc_esi = loc_esi>=7 ? 11:9;
						new_eax = var18;
						if (!CLI_ISCONTAINED(orgsource, size_sum, (char*)(new_eax + loc_ecx), 1))
							return -1;
						var1 = *(uint8_t *)(new_eax + loc_ecx);
						loc_ecx = (loc_ecx&0xffffff00) | var1;
						/* loc_4865af */
						new_edx = var08++;
						if (!CLI_ISCONTAINED(orgsource, size_sum, (char*)(new_eax + new_edx), 1))
							return -1;
						*(uint8_t *)(new_eax + new_edx) = loc_ecx & 0xff;

						new_ecx = (char*)loc_ecx;

						/* loc_4866fe */
						new_eax = var08;
						continue; /* !!! */
					}

				}
				/* loc_4865fe */
				new_ecx = new_ebx + 0xa68;
				if (lzma_4863da (var0C, &var40, &new_ecx, &new_edx, &new_eax, orgsource, size_sum) == -1)
					return -1;
				var0C = new_eax;
				/* new_eax = (((loc_esi >= 7)-1)&0xFFFFFFFD) + 0xB; */
				new_eax = loc_esi>=7 ? 11:8;
				loc_esi = new_eax;
			}
			/* loc_4866cd */
			if (!loc_edi)
			{
				break;
			} else {
				var0C += 2;
				new_ecx = (char*)var18;
				new_edx = new_eax = var08;
				new_eax -= loc_edi;
				if ( ((var0C < var28 - new_edx) &&
						(!CLI_ISCONTAINED(orgsource, size_sum, (char*)(new_ecx + new_eax), var0C) || 
						 !CLI_ISCONTAINED(orgsource, size_sum, (char*)(new_ecx + new_edx), var0C))) ||
						(!CLI_ISCONTAINED(orgsource, size_sum, (char*)(new_ecx + new_eax), var28 - new_edx) ||
						 !CLI_ISCONTAINED(orgsource, size_sum, (char*)(new_ecx + new_edx), var28 - new_edx)) )
					return -1;
				do {
					var1 = *(uint8_t *)(new_ecx + new_eax);
					*(uint8_t *)(new_ecx + new_edx) = var1;

					new_edx++;
					new_eax++;
					var0C--;
					if (var0C <= 0)
						break;
				} while (new_edx < var28);
				var08 = new_edx;
			}
		} else {
			/* loc_4864C8 */
			new_eax = (((var1 & 0xff) >> 4)*3) << 9;
			new_ecx = new_eax + new_ebx + 0xe6c;
			var0C_ecxcopy = new_ecx;
			if (loc_esi >= 4)
			{
				/* loc_4864e8 */
				if (loc_esi >= 10)
					loc_esi -= 6;
				else
					loc_esi -= 3;

			} else {
				/* loc_4864e4 */
				loc_esi = 0;
			}

			if (var20 == 0)	{
				/* loc_48651D */
				new_eax = 1;
				do {
					/* loc_486525 */
					/*new_ecx = var0C_ecxcopy;*/
					new_eax += new_eax;
					new_ecx += new_eax;
					var34 = new_eax;
					if ((new_eax = lzma_486248(&var40, &new_ecx, orgsource, size_sum)) == -1)
						return -1;
					new_eax |= var34;
					/* loc_486522 */
					/* keeping it here instead of at the top
					 * seems to work faster
					 */
					if (new_eax < 0x100)
					{
						new_ecx = var0C_ecxcopy;
					}
				} while (new_eax < 0x100);
				/* loc_48653e */
				var1 = (uint8_t)(new_eax & 0xff);
			} else {
				int t;
				/* loc_4864FB */
				new_edx = var18;
				new_eax = var08 - loc_edi;
				if (!CLI_ISCONTAINED(orgsource, size_sum, (char*)(new_eax + new_edx), 1))
					return -1;
				t = *(uint8_t *)(new_eax+new_edx);
				new_eax = (new_eax&0xffffff00) | t;
				/*new_edx = (uint32_t)&var40;*/
				var30 = t;
				if (lzma_48635C (t, &new_ecx, &var40, &new_eax, orgsource, size_sum) == -1)
					return -1;
				var20 = 0;
				var1 = new_eax&0xff;
			}

			/* loc_486541 */
			new_eax = var18;
			/* unneeded: new_ecx = (new_ecx&0xffffff00) | var1; */

			/* loc_4865af */
			new_edx = var08++;

			if (!CLI_ISCONTAINED(orgsource, size_sum, (char*)(new_eax + new_edx), 1))
				return -1;
			*(uint8_t *)(new_eax + new_edx) = var1;
		}
		/* loc_4866fe */
		new_eax = var08;
	} while (new_eax < var28);

    	if (special) {
		uint32_t loc_ecx;
		/* let's fix calls */
		loc_ecx = 0;
		cli_dbgmsg("MEWlen: %08x ? %08x\n", new_edx, pushed_edx);

		if (!CLI_ISCONTAINED(orgsource, size_sum, pushed_esi, pushed_edx))
			return -1;
		do {
			/* 0xe8, 0xe9 call opcodes */
			if (pushed_esi[loc_ecx] == '\xe8' || pushed_esi[loc_ecx] == '\xe9')
			{
				char *adr = (char *)(pushed_esi + loc_ecx + 1);
				loc_ecx++;
				
				cli_writeint32(adr, EC32(CE32((uint32_t)cli_readint32(adr)))-loc_ecx);
				loc_ecx += 4;
			} else 
				loc_ecx++;
		} while (loc_ecx != pushed_edx);
		return 0; /*pushed_edx;*/
	}
    } while (mainloop);

    return 0xbadc0de;
}


/* UPack lzma */

/* compare with 486248 */
uint32_t lzma_upack_esi_00(struct lzmastate *p, char *old_ecx, char *bb, uint32_t bl)
{
	uint32_t loc_eax, ret, loc_edi;
	loc_eax = p->p1 >> 0xb;
	if (!CLI_ISCONTAINED(bb, bl, old_ecx, 4) || !CLI_ISCONTAINED(bb, bl, p->p0, 4))
	{
		if (!CLI_ISCONTAINED(bb, bl, old_ecx, 4))
			cli_dbgmsg("contain error! %08x %08x ecx: %08x [%08x]\n", bb, bl, old_ecx,bb+bl);
		else
			cli_dbgmsg("contain error! %08x %08x p0: %08x [%08x]\n", bb, bl, p->p0,bb+bl);
		return -1;
	}
	ret = cli_readint32(old_ecx);
	loc_eax *= ret;
	loc_edi = cli_readint32((char *)p->p0);
	loc_edi = EC32(CE32(loc_edi)); /* bswap */
	loc_edi -= p->p2;
	if (loc_edi < loc_eax)
	{
		p->p1 = loc_eax;
		loc_eax = (0x800 - ret) >> 5;
		cli_writeint32(old_ecx, cli_readint32(old_ecx) + loc_eax);
		ret = 0;
	} else {
		p->p2 += loc_eax;
		p->p1 -= loc_eax;
		loc_eax = ret >> 5;
		cli_writeint32(old_ecx, cli_readint32(old_ecx) - loc_eax);
		ret = 1;
	}
	if(((p->p1)&0xff000000) == 0)
	{
		p->p2 <<= 8;
		p->p1 <<= 8;
		p->p0++;
	}
	return ret;
}

/* compare with lzma_4862e0 */
/* lzma_upack_esi_4c 0x1 as eax!
 */
uint32_t lzma_upack_esi_50(struct lzmastate *p, uint32_t old_eax, uint32_t old_ecx, char **old_edx, char *old_ebp, uint32_t *retval, char *bs, uint32_t bl)
{
	uint32_t loc_eax = old_eax, original = old_eax, ret;

	do {
		*old_edx = old_ebp + (loc_eax<<2);
		if ((ret = lzma_upack_esi_00(p, *old_edx, bs, bl)) == -1)
			return -1;
		loc_eax += loc_eax;
		loc_eax += ret;
	} while (loc_eax < old_ecx);

/*	cli_dbgmsg("loc_eax: %08x - ecx: %08x = %08x || original: %08x\n", loc_eax, old_ecx, loc_eax - old_ecx, original); */
	*retval = loc_eax - old_ecx;
	return 0;
}

uint32_t lzma_upack_esi_54(struct lzmastate *p, uint32_t old_eax, uint32_t *old_ecx, char **old_edx, uint32_t *retval, char *bs, uint32_t bl)
{
	uint32_t ret, loc_eax = old_eax;

	*old_ecx = ((*old_ecx)&0xffffff00)|8;
	ret = lzma_upack_esi_00 (p, *old_edx, bs, bl);
	*old_edx = ((*old_edx) + 4);
	loc_eax = (loc_eax&0xffffff00)|1;
	if (ret)
	{
		ret = lzma_upack_esi_00 (p, *old_edx, bs, bl);
		loc_eax |= 8; /* mov al, 9 */
		if (ret)
		{
			*old_ecx <<= 5;
			loc_eax = 0x11; /* mov al, 11 */
		}
	}
	ret = loc_eax;
	if (lzma_upack_esi_50(p, 1, *old_ecx, old_edx, *old_edx + (loc_eax << 2), &loc_eax, bs, bl) == -1)
		return -1;

	*retval = ret + loc_eax;
	return 0;
}


int unmew11(struct pe_image_section_hdr *section_hdr, int sectnum, char *src, int off, int ssize, int dsize, uint32_t base, uint32_t vadd, int uselzma, char **endsrc, char **enddst, int filedesc)
{
	uint32_t entry_point, newedi, loc_ds=dsize, loc_ss=ssize;
	char *source = src + dsize + off; /*EC32(section_hdr[sectnum].VirtualSize) + off;*/
	char *lesi = source + 12, *ledi;
	char *f1, *f2;
	int i;
	struct cli_exe_section *section = NULL;
	uint32_t vma = base + vadd, size_sum = ssize + dsize;

	entry_point  = cli_readint32(source + 4); /* 2vGiM: ate these safe enough?
						   * yup, if (EC32(section_hdr[i + 1].SizeOfRawData) < ...
						   * ~line #879 in pe.c
						   */
	newedi = cli_readint32(source + 8);
	ledi = src + (newedi - vma);

	i = 0;
	ssize -= 12;
	while (1)
	{
  		cli_dbgmsg("MEW unpacking section %d (%08x->%08x)\n", i, lesi, ledi);
		if (!CLI_ISCONTAINED(src, size_sum, lesi, 4) || !CLI_ISCONTAINED(src, size_sum, ledi, 4))
		{
			cli_dbgmsg("Possibly programmer error or hand-crafted PE file, report to clamav team\n");
			return -1;
		}
		if (unmew(lesi, ledi, loc_ss, loc_ds, &f1, &f2))
		{
			free(section);
			return -1;
		}

		/* we don't need last section in sections since this is information for fixing imptbl */
		if (!CLI_ISCONTAINED(src, size_sum, f1, 4))
		{
			free(section);
			return -1;
		}

		/* XXX */
		loc_ss -= (f1+4-lesi);
		loc_ds -= (f2-ledi);
		ledi = src + (cli_readint32(f1) - vma);
		lesi = f1+4;

		if (!uselzma)
		{
			uint32_t val = f2 - src;
			/* round-up to 4k boundary, I'm not sure of this XXX */
			val >>= 12;
			val <<= 12;
			val += 0x1000;

			/* eeevil XXX */
			section = cli_realloc(section, (i+2)*sizeof(struct cli_exe_section));
			section[0].raw = 0; section[0].rva = vadd;
			section[i+1].raw = val;
			section[i+1].rva = val + vadd;
			section[i].rsz = section[i].vsz = i?val - section[i].raw:val;
		}
		i++;

		if (!cli_readint32(f1))
			break;
	}

	/* LZMA stuff */
	if (uselzma) {
		/* put everything in one section */
		i = 1;
		if (!CLI_ISCONTAINED(src, size_sum, src+uselzma+8, 1))
		{
			cli_dbgmsg("MEW: couldn't access lzma 'special' tag\n");
			free(section);
			return -1;
		}
		/* 0x50 -> push eax */
		cli_dbgmsg("MEW: lzma %swas used, unpacking\n", (*(src + uselzma+8) == '\x50')?"special ":"");
		if (!CLI_ISCONTAINED(src, size_sum, f1+4, 20 + 4 + 5))
		{
			cli_dbgmsg("MEW: lzma initialization data not available!\n");
			free(section);
			return -1;
		}
		if(mew_lzma(&(section_hdr[sectnum]), src, f1+4, size_sum, vma, *(src + uselzma+8) == '\x50'))
		{
			free(section);
			return -1;
		}
		loc_ds >>= 12; loc_ds <<= 12; loc_ds += 0x1000;
		/* I have EP but no section's information, so I weren't sure what to do with that */ /* 2vGiM: sounds fair */
		section = cli_calloc(1, sizeof(struct cli_exe_section));
		section[0].raw = 0; section[0].rva = vadd;
		section[0].rsz = section[0].vsz = dsize;
	}
	if (!cli_rebuildpe(src, section, i, base, entry_point - base, 0, 0, filedesc))
	{
		cli_dbgmsg("MEW: Rebuilding failed\n");
		return -1;
	}

	return 1;
}

#endif /* CL_EXPERIMENTAL */
