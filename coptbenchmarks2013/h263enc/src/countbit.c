/************************************************************************
 *
 *  countbit.c, bitstream generation for tmn (TMN encoder)
 *  Copyright (C) 1996  Telenor R&D, Norway
 *        Karl Olav Lillevold <Karl.Lillevold@nta.no>
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Karl Olav Lillevold               <Karl.Lillevold@nta.no>
 *  Telenor Research and Development
 *  P.O.Box 83                        tel.:   +47 63 84 84 00
 *  N-2007 Kjeller, Norway            fax.:   +47 63 81 00 76
 *
 *  Robert Danielsen                  e-mail: Robert.Danielsen@nta.no
 *  Telenor Research and Development  www:    http://www.nta.no/brukere/DVC/
 *  P.O.Box 83                        tel.:   +47 63 84 84 00
 *  N-2007 Kjeller, Norway            fax.:   +47 63 81 00 76
 *  
 ************************************************************************/

#include"sim.h"
#include"sactbls.h"
#include"indices.h"
#include "putvlc.h"
 
int arith_used = 0;

/**********************************************************************
 *
 *	Name:        CountBitsMB
 *	Description:    counts bits used for MB info
 *	
 *	Input:	        Mode, COD, CBP, Picture and Bits structures
 *	Returns:       
 *	Side effects:
 *
 *	Date: 941129	Author: Karl.Lillevold@nta.no
 *
 ***********************************************************************/

void CountBitsMB(int Mode, int COD, int CBP, int CBPB, Pict *pic, Bits *bits)
{
  int cbpy, cbpcm, length;

  /* COD */
  if (trace) {
    fprintf(tf,"MB-nr: %d",pic->MB);
    if (pic->picture_coding_type == PCT_INTER)
      fprintf(tf,"  COD: %d\n",COD);
  }
  if (pic->picture_coding_type == PCT_INTER) {
    putbits(1,COD);
    bits->COD++;
  }

  if (COD) 
    return;    /* not coded */

  /* CBPCM */
  cbpcm = Mode | ((CBP&3)<<4);
  if (trace) {
    fprintf(tf,"CBPCM (CBP=%d) (cbpcm=%d): ",CBP,cbpcm);
  }
  if (pic->picture_coding_type == PCT_INTRA)
    length = put_cbpcm_intra (CBP, Mode);
  else
    length = put_cbpcm_inter (CBP, Mode);
  bits->CBPCM += length;

    /* MODB & CBPB */
  if (pic->PB) {
    switch (pic->MODB) {
    case PBMODE_NORMAL:
      putbits(1,0);
      bits->MODB += 1;
      break;
    case PBMODE_MVDB:
      putbits(2,2);
      bits->MODB += 2;
      break;
    case PBMODE_CBPB_MVDB:
      putbits(2,3);
      bits->MODB += 2;
      /* CBPB */
      putbits(6,CBPB);
      bits->CBPB += 6;
      break;
    }
    if (trace) 
      fprintf(tf,"MODB: %d, CBPB: %d\n", pic->MODB, CBPB);
  }
    
  /* CBPY */
  cbpy = CBP>>2;
  if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q) /* Intra */
    cbpy = cbpy^15;
  if (trace) {
    fprintf(tf,"CBPY (CBP=%d) (cbpy=%d): ",CBP,cbpy);
  }
  length = put_cbpy (CBP, Mode);

  bits->CBPY += length;
  
  /* DQUANT */
  if ((Mode == MODE_INTER_Q) || (Mode == MODE_INTRA_Q)) {
    if (trace) {
      fprintf(tf,"DQUANT: ");
    }
    switch (pic->DQUANT) {
    case -1:
      putbits(2,0);
      break;
    case -2:
      putbits(2,1);
      break;
    case 1:
      putbits(2,2);
      break;
    case 2:
      putbits(2,3);
      break;
    default:
      fprintf(stderr,"Invalid DQUANT\n");
      exit(-1);
    }
    bits->DQUANT += 2;
  }
  return;
}

/**********************************************************************
 *
 *      Name:           Count_sac_BitsMB
 *      Description:    counts bits used for MB info using SAC models
 *                      modified from CountBitsMB
 *
 *      Input:          Mode, COD, CBP, Picture and Bits structures
 *      Returns:	none
 *      Side effects:	Updates Bits structure.
 *
 *      Author:        pmulroy@visual.bt.co.uk
 *
 ***********************************************************************/
 
void Count_sac_BitsMB(int Mode,int COD,int CBP,int CBPB,Pict *pic,Bits *bits)
{
  int cbpy, cbpcm, length, i;
 
  arith_used = 1;
 
  /* COD */
 
  if (trace) {
    fprintf(tf,"MB-nr: %d",pic->MB);
    if (pic->picture_coding_type == PCT_INTER)
      fprintf(tf,"  COD: %d ",COD);
  }
 
  if (pic->picture_coding_type == PCT_INTER)
    bits->COD+=AR_Encode(COD, cumf_COD);
 
  if (COD)
    return;    /* not coded */
 
  /* CBPCM */
 
  cbpcm = Mode | ((CBP&3)<<4);
  if (trace) {
    fprintf(tf,"CBPCM (CBP=%d) (cbpcm=%d): ",CBP,cbpcm);
  }
  if (pic->picture_coding_type == PCT_INTRA)
    length = AR_Encode(indexfn(cbpcm,mcbpc_intratab,9),cumf_MCBPC_intra);
  else
    length = AR_Encode(indexfn(cbpcm,mcbpctab,21),cumf_MCBPC);
 
  bits->CBPCM += length;
 
  /* MODB & CBPB */
   if (pic->PB) {
     switch (pic->MODB) {
     case PBMODE_NORMAL:
       bits->MODB += AR_Encode(0, cumf_MODB);
       break;
     case PBMODE_MVDB:
       bits->MODB += AR_Encode(1, cumf_MODB);
       break;
     case PBMODE_CBPB_MVDB:
       bits->MODB += AR_Encode(2, cumf_MODB);
       /* CBPB */
       for(i=5; i>1; i--)
         bits->CBPB += AR_Encode(((CBPB & 1<<i)>>i), cumf_YCBPB);
       for(i=1; i>-1; i--)
         bits->CBPB += AR_Encode(((CBPB & 1<<i)>>i), cumf_UVCBPB);
       break;
     }
     if (trace) 
       fprintf(tf,"MODB: %d, CBPB: %d\n", pic->MODB, CBPB);
   }

  /* CBPY */
 
  cbpy = CBP>>2;
  if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q) { /* Intra */
    length = AR_Encode(indexfn(cbpy,cbpy_intratab,16),cumf_CBPY_intra);
  } else {
    length = AR_Encode(indexfn(cbpy,cbpytab,16),cumf_CBPY);
  }
  if (trace) {
    fprintf(tf,"CBPY (CBP=%d) (cbpy=%d): ",CBP,cbpy);
  }
  bits->CBPY += length;
 
  /* DQUANT */
 
  if ((Mode == MODE_INTER_Q) || (Mode == MODE_INTRA_Q)) {
    if (trace) {
      fprintf(tf,"DQUANT: ");
    }
    bits->DQUANT += AR_Encode(indexfn(pic->DQUANT+2,dquanttab,4), cumf_DQUANT);
  }
  return;
}


/**********************************************************************
 *
 *	Name:        CountBitsSlice
 *	Description:    couonts bits used for slice (GOB) info
 *	
 *	Input:	        slice no., quantizer
 *
 *	Date: 94????	Author: Karl.Lillevold@nta.no
 *
 ***********************************************************************/

int CountBitsSlice(int slice, int quant)
{
  int bits = 0;

  if (arith_used) {
    bits+=encoder_flush(); /* Need to call before fixed length string output */
    arith_used = 0;
  }

  /* Picture Start Code */
  if (trace)
    fprintf(tf,"GOB sync (GBSC): ");
  putbits(PSC_LENGTH,PSC); /* PSC */
  bits += PSC_LENGTH;

  /* Group Number */
  if (trace)
    fprintf(tf,"GN: ");
  putbits(5,slice);
  bits += 5;

  /* GOB Sub Bitstream Indicator */
  /* if CPM == 1: read 2 bits GSBI */
  /* not supported in this version */

  /* GOB Frame ID */
  if (trace)
    fprintf(tf,"GFID: ");
  putbits(2, 0);  
  /* NB: in error-prone environments this value should change if 
     PTYPE in picture header changes. In this version of the encoder
     PTYPE only changes when PB-frames are used in the following cases:
     (i) after the first intra frame
     (ii) if the distance between two P-frames is very large 
     Therefore I haven't implemented this GFID change */
  /* GFID is not allowed to change unless PTYPE changes */
  bits += 2;

  /* Gquant */
  if (trace)
    fprintf(tf,"GQUANT: ");
  putbits(5,quant);
  bits += 5;

  return bits;
}


/**********************************************************************
 *
 *	Name:        CountBitsCoeff
 *	Description:	counts bits used for coeffs
 *	
 *	Input:        qcoeff, coding mode CBP, bits structure, no. of 
 *                      coeffs
 *        
 *	Returns:	struct with no. of bits used
 *	Side effects:	
 *
 *	Date: 940111	Author:	Karl.Lillevold@nta.no
 *
 ***********************************************************************/

void CountBitsCoeff(int *qcoeff, int Mode, int CBP, Bits *bits, int ncoeffs)
{
  
  int i;

  if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q) {
    for (i = 0; i < 4; i++) {
      bits->Y += CodeCoeff(Mode, qcoeff,i,ncoeffs);
    }
    for (i = 4; i < 6; i++) {
      bits->C += CodeCoeff(Mode, qcoeff,i,ncoeffs);
    }
  }
  else {
    for (i = 0; i < 4; i++) {
      if ((i==0 && CBP&32) || 
          (i==1 && CBP&16) ||
          (i==2 && CBP&8) || 
          (i==3 && CBP&4) || 
          (i==4 && CBP&2) ||
          (i==5 && CBP&1)) {
        bits->Y += CodeCoeff(Mode, qcoeff, i, ncoeffs);
      }
    }
    for (i = 4; i < 6; i++) {
      if ((i==0 && CBP&32) || 
          (i==1 && CBP&16) ||
          (i==2 && CBP&8) || 
          (i==3 && CBP&4) || 
          (i==4 && CBP&2) ||
          (i==5 && CBP&1)) {
        bits->C += CodeCoeff(Mode, qcoeff, i, ncoeffs);
      }
    }
  }
  return;
}
  
int CodeCoeff(int Mode, int *qcoeff, int block, int ncoeffs)
{
  int j, bits;
  int prev_run, run, prev_level, level, first;
  int prev_s, s, length;

  run = bits = 0;
  first = 1;
  prev_run = prev_level = level = s = prev_s = 0;
  
  if (trace) {
    fprintf(tf,"Coeffs block %d:\n",block);
  }

  for (j = block*ncoeffs; j< (block + 1)*ncoeffs; j++) {
    /* Do this block's DC-coefficient first */
    if (!(j%ncoeffs) && (Mode == MODE_INTRA || Mode == MODE_INTRA_Q)) {
      /* DC coeff */
      if (trace) {
        fprintf(tf,"DC: ");
      }
      if (qcoeff[block*ncoeffs] != 128)
        putbits(8,qcoeff[block*ncoeffs]);
      else
        putbits(8,255);
      bits += 8;
    }
    else {
      /* AC coeff */
      s = 0;
      /* Increment run if coeff is zero */
      if ((level = qcoeff[j]) == 0) {
        run++;
      }
      else {
        /* code run & level and count bits */
        if (level < 0) {
          s = 1;
          level = -level;
        }

        if (!first) {
          /* Encode the previous coefficient */
          if (prev_level  < 13 && prev_run < 64)
            length = put_coeff (prev_run, prev_level, 0);  
          else
            length = 0;
          if (length == 0) {  /* Escape coding */
            if (trace) {
              fprintf(tf,"Escape code: ");
            }
            if (prev_s == 1) {prev_level = (prev_level^0xff)+1;}
            putbits(7,3);	/* Escape code */
            if (trace)
              fprintf(tf,"last: ");
            putbits(1,0);
            if (trace)
              fprintf(tf,"run: ");
            putbits(6,prev_run);
            if (trace)
              fprintf(tf,"level: ");
            putbits(8,prev_level);
            bits += 22;
          }
          else {
            putbits(1,prev_s);
            bits += length + 1;
          }
        }
        prev_run = run; prev_s = s;
        prev_level = level; 

        run = first = 0;
      }
    }
  }
  /* Encode the last coeff */
  if (!first) {
    if (trace) {
      fprintf(tf,"Last coeff: ");
    }
    if (prev_level  < 13 && prev_run < 64) 
      length = put_coeff (prev_run, prev_level, 1);   
    else
      length = 0;
    if (length == 0) {  /* Escape coding */
      if (trace) {
        fprintf(tf,"Escape code: ");
      }
      if (prev_s == 1) {prev_level = (prev_level^0xff)+1;}
      putbits (7,3);	/* Escape code */
      if (trace)
        fprintf(tf,"last: ");
      putbits(1,1);
      if (trace)
        fprintf(tf,"run: ");
      putbits(6,prev_run);
      if (trace)
        fprintf(tf,"level: ");
      putbits(8,prev_level);
      bits += 22;
    }
    else {
      putbits(1,prev_s);
      bits += length + 1;
    }
  }
  return bits;
}

/**********************************************************************
 *
 *      Name:           Count_sac_BitsCoeff
 *                      counts bits using SAC models
 *
 *      Input:          qcoeff, coding mode CBP, bits structure, no. of
 *                      coeffs
 *
 *      Returns:        struct with no. of bits used
 *      Side effects:
 *
 *      Author:        pmulroy@visual.bt.co.uk
 *
 ***********************************************************************/
 
void Count_sac_BitsCoeff(int *qcoeff,int Mode,int CBP,Bits *bits,int ncoeffs)
{
 
  int i;
 
  arith_used = 1;
 
  if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q) {
    for (i = 0; i < 4; i++) {
      bits->Y += Code_sac_Coeff(Mode, qcoeff,i,ncoeffs);
    }
    for (i = 4; i < 6; i++) {
      bits->C += Code_sac_Coeff(Mode, qcoeff,i,ncoeffs);
    }
  }
  else {
    for (i = 0; i < 4; i++) {
      if ((i==0 && CBP&32) ||
          (i==1 && CBP&16) ||
          (i==2 && CBP&8) ||
          (i==3 && CBP&4) ||
          (i==4 && CBP&2) ||
          (i==5 && CBP&1)) {
        bits->Y += Code_sac_Coeff(Mode, qcoeff, i, ncoeffs);
      }
    }
    for (i = 4; i < 6; i++) {
      if ((i==0 && CBP&32) ||
          (i==1 && CBP&16) ||
          (i==2 && CBP&8) ||
          (i==3 && CBP&4) ||
          (i==4 && CBP&2) ||
          (i==5 && CBP&1)) {
        bits->C += Code_sac_Coeff(Mode, qcoeff, i, ncoeffs);
      }
    }
  }
  return;
}
 
int Code_sac_Coeff(int Mode, int *qcoeff, int block, int ncoeffs)
{
  int j, bits, mod_index, intra;
  int prev_run, run, prev_level, level, first, prev_position, position;
  int prev_ind, ind, prev_s, s, length;
 
  run = bits = 0;
  first = 1; position = 0; intra = 0;
 
  level = s = ind = 0;
  prev_run = prev_level = prev_ind = prev_s = prev_position = 0;
 
  intra = (Mode == MODE_INTRA || Mode == MODE_INTRA_Q);
 
  for (j = block*ncoeffs; j< (block + 1)*ncoeffs; j++) {
 
    if (!(j%ncoeffs) && intra) {
      if (trace) {
        fprintf(tf,"DC: ");
      }
      if (qcoeff[block*ncoeffs]!=128)
        mod_index = indexfn(qcoeff[block*ncoeffs],intradctab,254);
      else
        mod_index = indexfn(255,intradctab,254);
      bits += AR_Encode(mod_index, cumf_INTRADC);
    }
    else {
 
      s = 0;
      /* Increment run if coeff is zero */
      if ((level = qcoeff[j]) == 0) {
        run++;
      }
      else {
        /* code run & level and count bits */
        if (level < 0) {
          s = 1;
          level = -level;
        }
        ind = level | run<<4;
        ind = ind | 0<<12; /* Not last coeff */
        position++;
 
        if (!first) {
          mod_index = indexfn(prev_ind, tcoeftab, 103);
          if (prev_level < 13 && prev_run < 64)
            length = CodeTCoef(mod_index, prev_position, intra);
          else
            length = -1;
 
          if (length == -1) {  /* Escape coding */
            if (trace) {
              fprintf(tf,"Escape coding:\n");
            }
 
            if (prev_s == 1) {prev_level = (prev_level^0xff)+1;}
 
            mod_index = indexfn(ESCAPE, tcoeftab, 103);
            bits += CodeTCoef(mod_index, prev_position, intra);
 
            if (intra)
              bits += AR_Encode(indexfn(0, lasttab, 2), cumf_LAST_intra);
            else
              bits += AR_Encode(indexfn(0, lasttab, 2), cumf_LAST);
 
            if (intra)
              bits += AR_Encode(indexfn(prev_run, runtab, 64), cumf_RUN_intra);
            else
              bits += AR_Encode(indexfn(prev_run, runtab, 64), cumf_RUN);
 
            if (intra)
              bits += AR_Encode(indexfn(prev_level, leveltab, 254), 
        cumf_LEVEL_intra);
            else
              bits += AR_Encode(indexfn(prev_level, leveltab, 254), 
        cumf_LEVEL);
 
          }
          else {
            bits += AR_Encode(indexfn(prev_s, signtab, 2), cumf_SIGN);
            bits += length;
          }
        }
 
        prev_run = run; prev_s = s;
        prev_level = level; prev_ind = ind;
        prev_position = position;
 
        run = first = 0;
 
      }
    }
  }
 
  /* Encode Last Coefficient */
 
  if (!first) {
    if (trace) {
      fprintf(tf,"Last coeff: ");
    }
    prev_ind = prev_ind | 1<<12;   /* last coeff */
    mod_index = indexfn(prev_ind, tcoeftab, 103);
 
    if (prev_level  < 13 && prev_run < 64)
      length = CodeTCoef(mod_index, prev_position, intra);
    else
      length = -1;
 
    if (length == -1) {  /* Escape coding */
      if (trace) {
        fprintf(tf,"Escape coding:\n");
      }

      if (prev_s == 1) {prev_level = (prev_level^0xff)+1;}
 
      mod_index = indexfn(ESCAPE, tcoeftab, 103);
      bits += CodeTCoef(mod_index, prev_position, intra);
 
      if (intra)
        bits += AR_Encode(indexfn(1, lasttab, 2), cumf_LAST_intra);
      else
        bits += AR_Encode(indexfn(1, lasttab, 2), cumf_LAST);
 
      if (intra)
        bits += AR_Encode(indexfn(prev_run, runtab, 64), cumf_RUN_intra);
      else
        bits += AR_Encode(indexfn(prev_run, runtab, 64), cumf_RUN);
 
      if (intra)
        bits += AR_Encode(indexfn(prev_level, leveltab, 254), cumf_LEVEL_intra);
      else
        bits += AR_Encode(indexfn(prev_level, leveltab, 254), cumf_LEVEL);
    }
    else {
      bits += AR_Encode(indexfn(prev_s, signtab, 2), cumf_SIGN);
      bits += length;
    }
  } /* last coeff */
 
  return bits;
}
 
/*********************************************************************
 *
 *      Name:           CodeTCoef
 *
 *      Description:    Encodes an AC Coefficient using the
 *                      relevant SAC model.
 *
 *      Input:          Model index, position in DCT block and intra/
 *        inter flag.
 *
 *      Returns:        Number of bits used.
 *
 *      Side Effects:   None
 *
 *      Author:         pmulroy@visual.bt.co.uk
 *
 *********************************************************************/

int CodeTCoef(int mod_index, int position, int intra)
{
  int length;
 
  switch (position) {
    case 1:
    {
        if (intra)
          length = AR_Encode(mod_index, cumf_TCOEF1_intra);
        else
          length = AR_Encode(mod_index, cumf_TCOEF1);
        break;
    }
    case 2:
    {
        if (intra)
          length = AR_Encode(mod_index, cumf_TCOEF2_intra);
        else
          length = AR_Encode(mod_index, cumf_TCOEF2);
        break;
    }
    case 3:
    {
        if (intra)
          length = AR_Encode(mod_index, cumf_TCOEF3_intra);
        else
          length = AR_Encode(mod_index, cumf_TCOEF3);
        break;
    }
    default:
    {
        if (intra)
          length = AR_Encode(mod_index, cumf_TCOEFr_intra);
        else
          length = AR_Encode(mod_index, cumf_TCOEFr);
        break;
    }
  }
 
  return length;
}

/**********************************************************************
 *
 *	Name:        FindCBP
 *	Description:	Finds the CBP for a macroblock
 *	
 *	Input:        qcoeff and mode
 *        
 *	Returns:	CBP
 *	Side effects:	
 *
 *	Date: 940829	Author:	Karl.Lillevold@nta.no
 *
 ***********************************************************************/


int FindCBP(int *qcoeff, int Mode, int ncoeffs)
{
  
  int i,j;
  int CBP = 0;
  int intra = (Mode == MODE_INTRA || Mode == MODE_INTRA_Q);

  /* Set CBP for this Macroblock */
  for (i = 0; i < 6; i++) {
    for (j = i*ncoeffs + intra; j < (i+1)*ncoeffs; j++) {
      if (qcoeff[j]) {
        if (i == 0) {CBP |= 32;}
        else if (i == 1) {CBP |= 16;}
        else if (i == 2) {CBP |= 8;}
        else if (i == 3) {CBP |= 4;}
        else if (i == 4) {CBP |= 2;}
        else if (i == 5) {CBP |= 1;}
        else {
          fprintf(stderr,"Error in CBP assignment\n");
          exit(-1);
        }
        break;
      }
    }
  }

  return CBP;
}


void CountBitsVectors(MotionVector *MV[6][MBR+1][MBC+2], Bits *bits, 
              int x, int y, int Mode, int newgob, Pict *pic)
{
  int y_vec, x_vec;
  int pmv0, pmv1;
  int start,stop,block;

  x++;y++;

  if (Mode == MODE_INTER4V) {
    start = 1; stop = 4;
  }
  else {
    start = 0; stop = 0;
  }

  for (block = start; block <= stop;  block++) {

    FindPMV(MV,x,y,&pmv0,&pmv1, block, newgob, 1);

    x_vec = (2*MV[block][y][x]->x + MV[block][y][x]->x_half) - pmv0;
    y_vec = (2*MV[block][y][x]->y + MV[block][y][x]->y_half) - pmv1;

    if (!long_vectors) {
      if (x_vec < -32) x_vec += 64;
      else if (x_vec > 31) x_vec -= 64;

      if (y_vec < -32) y_vec += 64;
      else if (y_vec > 31) y_vec -= 64;
    }
    else {
      if (pmv0 < -31 && x_vec < -63) x_vec += 64;
      else if (pmv0 > 32 && x_vec > 63) x_vec -= 64;

      if (pmv1 < -31 && y_vec < -63) y_vec += 64;
      else if (pmv1 > 32 && y_vec > 63) y_vec -= 64;
    }
    
    if (trace) {
      fprintf(tf,"Vectors:\n");
    }

    if (x_vec < 0) x_vec += 64;
    if (y_vec < 0) y_vec += 64;

    bits->vec += put_mv (x_vec);
    bits->vec += put_mv (y_vec);

    if (trace) {
      if (x_vec > 31) x_vec -= 64;
      if (y_vec > 31) y_vec -= 64;
      fprintf(tf,"(x,y) = (%d,%d) - ",
              (2*MV[block][y][x]->x + MV[block][y][x]->x_half),
              (2*MV[block][y][x]->y + MV[block][y][x]->y_half));
      fprintf(tf,"(Px,Py) = (%d,%d)\n", pmv0,pmv1);
      fprintf(tf,"(x_diff,y_diff) = (%d,%d)\n",x_vec,y_vec);
    }
  }

  /* PB-frames delta vectors */
  if (pic->PB)
    if (pic->MODB == PBMODE_MVDB || pic->MODB == PBMODE_CBPB_MVDB) {

      x_vec = MV[5][y][x]->x;
      y_vec = MV[5][y][x]->y;

      /* x_vec and y_vec are the PB-delta vectors */
    
      if (x_vec < 0) x_vec += 64;
      if (y_vec < 0) y_vec += 64;

      if (trace) {
        fprintf(tf,"PB delta vectors:\n");
      }

      bits->vec += put_mv (x_vec);
      bits->vec += put_mv (y_vec);

      if (trace) {
        if (x_vec > 31) x_vec -= 64;
        if (y_vec > 31) y_vec -= 64;
        fprintf(tf,"MVDB (x,y) = (%d,%d)\n",x_vec,y_vec);
      }
    }


  return;
}

void Count_sac_BitsVectors(MotionVector *MV[6][MBR+1][MBC+2], Bits *bits,
                      int x, int y, int Mode, int newgob, Pict *pic)
{
  int y_vec, x_vec;
  int pmv0, pmv1;
  int start,stop,block;
 
  arith_used = 1;
  x++;y++;
 
  if (Mode == MODE_INTER4V) {
    start = 1; stop = 4;
  }
  else {
    start = 0; stop = 0;
  }
 
  for (block = start; block <= stop;  block++) {
 
    FindPMV(MV,x,y,&pmv0,&pmv1, block, newgob, 1);
 
    x_vec = (2*MV[block][y][x]->x + MV[block][y][x]->x_half) - pmv0;
    y_vec = (2*MV[block][y][x]->y + MV[block][y][x]->y_half) - pmv1;
 
    if (!long_vectors) {
      if (x_vec < -32) x_vec += 64;
      else if (x_vec > 31) x_vec -= 64;

      if (y_vec < -32) y_vec += 64;
      else if (y_vec > 31) y_vec -= 64;
    }
    else {
      if (pmv0 < -31 && x_vec < -63) x_vec += 64;
      else if (pmv0 > 32 && x_vec > 63) x_vec -= 64;

      if (pmv1 < -31 && y_vec < -63) y_vec += 64;
      else if (pmv1 > 32 && y_vec > 63) y_vec -= 64;
    }

    if (x_vec < 0) x_vec += 64;
    if (y_vec < 0) y_vec += 64;
 
    if (trace) {
      fprintf(tf,"Vectors:\n");
    }
 
    bits->vec += AR_Encode(indexfn(x_vec,mvdtab,64),cumf_MVD);
    bits->vec += AR_Encode(indexfn(y_vec,mvdtab,64),cumf_MVD);
 
    if (trace) {
      if (x_vec > 31) x_vec -= 64;
      if (y_vec > 31) y_vec -= 64;
      fprintf(tf,"(x,y) = (%d,%d) - ",
              (2*MV[block][y][x]->x + MV[block][y][x]->x_half),
              (2*MV[block][y][x]->y + MV[block][y][x]->y_half));
      fprintf(tf,"(Px,Py) = (%d,%d)\n", pmv0,pmv1);
      fprintf(tf,"(x_diff,y_diff) = (%d,%d)\n",x_vec,y_vec);
    }
  }

   /* PB-frames delta vectors */
  if (pic->PB)
    if (pic->MODB == PBMODE_MVDB || pic->MODB == PBMODE_CBPB_MVDB) {
 
      x_vec = MV[5][y][x]->x;
      y_vec = MV[5][y][x]->y;
 
      if (x_vec < -32)
        x_vec += 64;
      else if (x_vec > 31)
        x_vec -= 64;
      if (y_vec < -32)
        y_vec += 64;
      else if (y_vec > 31)
        y_vec -= 64;
      
      if (x_vec < 0) x_vec += 64;
      if (y_vec < 0) y_vec += 64;
      
      if (trace) {
        fprintf(tf,"PB delta vectors:\n");
      }
      
      bits->vec += AR_Encode(indexfn(x_vec,mvdtab,64),cumf_MVD);
      bits->vec += AR_Encode(indexfn(y_vec,mvdtab,64),cumf_MVD);
      
      if (trace) {
        if (x_vec > 31) x_vec -= 64;
        if (y_vec > 31) y_vec -= 64;
        fprintf(tf,"MVDB (x,y) = (%d,%d)\n",x_vec,y_vec);
      }
    }
  
  return;
}

void FindPMV(MotionVector *MV[6][MBR+1][MBC+2], int x, int y, 
             int *pmv0, int *pmv1, int block, int newgob, int half_pel)
{
  int p1,p2,p3;
  int xin1,xin2,xin3;
  int yin1,yin2,yin3;
  int vec1,vec2,vec3;
  int l8,o8,or8;


  l8 = o8 = or8 = 0;
  if (MV[0][y][x-1]->Mode == MODE_INTER4V)
    l8 = 1;
  if (MV[0][y-1][x]->Mode == MODE_INTER4V)
    o8 = 1;
  if (MV[0][y-1][x+1]->Mode == MODE_INTER4V)
    or8 = 1;

  switch (block) {
  case 0: 
    vec1 = (l8 ? 2 : 0) ; yin1 = y  ; xin1 = x-1;
    vec2 = (o8 ? 3 : 0) ; yin2 = y-1; xin2 = x;
    vec3 = (or8? 3 : 0) ; yin3 = y-1; xin3 = x+1;
    break;
  case 1:
    vec1 = (l8 ? 2 : 0) ; yin1 = y  ; xin1 = x-1;
    vec2 = (o8 ? 3 : 0) ; yin2 = y-1; xin2 = x;
    vec3 = (or8? 3 : 0) ; yin3 = y-1; xin3 = x+1;
    break;
  case 2:
    vec1 = 1            ; yin1 = y  ; xin1 = x;
    vec2 = (o8 ? 4 : 0) ; yin2 = y-1; xin2 = x;
    vec3 = (or8? 3 : 0) ; yin3 = y-1; xin3 = x+1;
    break;
  case 3:
    vec1 = (l8 ? 4 : 0) ; yin1 = y  ; xin1 = x-1;
    vec2 = 1            ; yin2 = y  ; xin2 = x;
    vec3 = 2            ; yin3 = y  ; xin3 = x;
    break;
  case 4:
    vec1 = 3            ; yin1 = y  ; xin1 = x;
    vec2 = 1            ; yin2 = y  ; xin2 = x;
    vec3 = 2            ; yin3 = y  ; xin3 = x;
    break;
  default:
    fprintf(stderr,"Illegal block number in FindPMV (countbit.c)\n");
    exit(-1);
    break;
  }
  if (half_pel) {
    p1 = 2*MV[vec1][yin1][xin1]->x + MV[vec1][yin1][xin1]->x_half;
    p2 = 2*MV[vec2][yin2][xin2]->x + MV[vec2][yin2][xin2]->x_half;
    p3 = 2*MV[vec3][yin3][xin3]->x + MV[vec3][yin3][xin3]->x_half;
  }
  else {
    p1 = 2*MV[vec1][yin1][xin1]->x;
    p2 = 2*MV[vec2][yin2][xin2]->x;
    p3 = 2*MV[vec3][yin3][xin3]->x;
  }
  if (newgob && (block == 0 || block == 1 || block == 2))
    p2 = 2*NO_VEC;

  if (p2 == 2*NO_VEC) { p2 = p3 = p1; }

  *pmv0 = p1+p2+p3 - mmax(p1,mmax(p2,p3)) - mmin(p1,mmin(p2,p3));
    
  if (half_pel) {
    p1 = 2*MV[vec1][yin1][xin1]->y + MV[vec1][yin1][xin1]->y_half;
    p2 = 2*MV[vec2][yin2][xin2]->y + MV[vec2][yin2][xin2]->y_half;
    p3 = 2*MV[vec3][yin3][xin3]->y + MV[vec3][yin3][xin3]->y_half;
  }
  else {
    p1 = 2*MV[vec1][yin1][xin1]->y;
    p2 = 2*MV[vec2][yin2][xin2]->y;
    p3 = 2*MV[vec3][yin3][xin3]->y;
  }    
  if (newgob && (block == 0 || block == 1 || block == 2))
    p2 = 2*NO_VEC;

  if (p2 == 2*NO_VEC) { p2 = p3 = p1; }

  *pmv1 = p1+p2+p3 - mmax(p1,mmax(p2,p3)) - mmin(p1,mmin(p2,p3));
  
  return;
}

void ZeroBits(Bits *bits)
{
  bits->Y = 0;
  bits->C = 0;
  bits->vec = 0;
  bits->CBPY = 0;
  bits->CBPCM = 0;
  bits->MODB = 0;
  bits->CBPB = 0;
  bits->COD = 0;
  bits->DQUANT = 0;
  bits->header = 0;
  bits->total = 0;
  bits->no_inter = 0;
  bits->no_inter4v = 0;
  bits->no_intra = 0;
  return;
}
void ZeroRes(Results *res)
{
  res->SNR_l = (float)0;
  res->SNR_Cr = (float)0;
  res->SNR_Cb = (float)0;
  res->QP_mean = (float)0;
}
void AddBits(Bits *total, Bits *bits)
{
  total->Y += bits->Y;
  total->C += bits->C;
  total->vec += bits->vec;
  total->CBPY += bits->CBPY;
  total->CBPCM += bits->CBPCM;
  total->MODB += bits->MODB;
  total->CBPB += bits->CBPB;
  total->COD += bits->COD;
  total->DQUANT += bits->DQUANT;
  total->header += bits->header;
  total->total += bits->total;
  total->no_inter += bits->no_inter;
  total->no_inter4v += bits->no_inter4v;
  total->no_intra += bits->no_intra;
  return;
}
void AddRes(Results *total, Results *res, Pict *pic)
{
  total->SNR_l += res->SNR_l;
  total->SNR_Cr += res->SNR_Cr;
  total->SNR_Cb += res->SNR_Cb;
  total->QP_mean += pic->QP_mean;
  return;
}

void AddBitsPicture(Bits *bits)
{
  bits->total = 
    bits->Y + 
    bits->C + 
    bits->vec +  
    bits->CBPY + 
    bits->CBPCM + 
    bits->MODB +
    bits->CBPB +
    bits->COD + 
    bits->DQUANT +
    bits->header ;
} 
void ZeroVec(MotionVector *MV)
{
  MV->x = 0;
  MV->y = 0;
  MV->x_half = 0;
  MV->y_half = 0;
  return;
}
void MarkVec(MotionVector *MV)
{
  MV->x = NO_VEC;
  MV->y = NO_VEC;
  MV->x_half = 0;
  MV->y_half = 0;
  return;
}

void CopyVec(MotionVector *MV2, MotionVector *MV1)
{
  MV2->x = MV1->x;
  MV2->x_half = MV1->x_half;
  MV2->y = MV1->y;
  MV2->y_half = MV1->y_half;
  return;
}

int EqualVec(MotionVector *MV2, MotionVector *MV1)
{
  if (MV1->x != MV2->x)
    return 0;
  if (MV1->y != MV2->y)
    return 0;
  if (MV1->x_half != MV2->x_half)
    return 0;
  if (MV1->y_half != MV2->y_half)
    return 0;
  return 1;
}

/**********************************************************************
 *
 *	Name:        CountBitsPicture(Pict *pic)
 *	Description:    counts the number of bits needed for picture
 *                      header
 *	
 *	Input:	        pointer to picture structure
 *	Returns:        number of bits
 *	Side effects:
 *
 *	Date: 941128	Author:Karl.Lillevold@nta.no
 *
 ***********************************************************************/

int CountBitsPicture(Pict *pic)
{
  int bits = 0;

  /* in case of arithmetic coding, encoder_flush() has been called before
     zeroflush() in main.c */

  /* Picture start code */
  if (trace) {
    fprintf(tf,"picture_start_code: ");
  }
  putbits(PSC_LENGTH,PSC);
  bits += PSC_LENGTH;

  /* Group number */
  if (trace) {
    fprintf(tf,"Group number in picture header: ");
  }
  putbits(5,0); 
  bits += 5;
  
  /* Time reference */
  if (trace) {
    fprintf(tf,"Time reference: ");
  }
  putbits(8,pic->TR);
  bits += 8;

 /* bit 1 */
  if (trace) {
    fprintf(tf,"spare: ");
  }
  pic->spare = 1; /* always 1 to avoid start code emulation */
  putbits(1,pic->spare);
  bits += 1;

  /* bit 2 */
  if (trace) {
    fprintf(tf,"always zero for distinction with H.261\n");
  }
  putbits(1,0);
  bits += 1;
  
  /* bit 3 */
  if (trace) {
    fprintf(tf,"split_screen_indicator: ");
  }
  putbits(1,0);     /* no support for split-screen in this software */
  bits += 1;

  /* bit 4 */
  if (trace) {
    fprintf(tf,"document_camera_indicator: ");
  }
  putbits(1,0);
  bits += 1;

  /* bit 5 */
  if (trace) {
    fprintf(tf,"freeze_picture_release: ");
  }
  putbits(1,0);
  bits += 1;

  /* bit 6-8 */
  if (trace) {
    fprintf(tf,"source_format: ");
  }
  putbits(3,pic->source_format);
  bits += 3;

  /* bit 9 */
  if (trace) {
    fprintf(tf,"picture_coding_type: ");
  }
  putbits(1,pic->picture_coding_type);
  bits += 1;

  /* bit 10 */
  if (trace) {
    fprintf(tf,"mv_outside_frame: ");
  }
  putbits(1,pic->unrestricted_mv_mode);  /* Unrestricted Motion Vector mode */
  bits += 1;

  /* bit 11 */
  if (trace) {
    fprintf(tf,"sac_coding: ");
  }
  putbits(1,syntax_arith_coding); /* Syntax-based Arithmetic Coding mode */
  bits += 1;

  /* bit 12 */
  if (trace) {
    fprintf(tf,"adv_pred_mode: ");
  }
  putbits(1,advanced); /* Advanced Prediction mode */
  bits += 1;

  /* bit 13 */
  if (trace) {
    fprintf(tf,"PB-coded: "); /* PB-frames mode */
  }
  putbits(1,pic->PB);
  bits += 1;


  /* QUANT */
  if (trace) {
    fprintf(tf,"QUANT: ");
  }
  putbits(5,pic->QUANT);
  bits += 5;

  /* Continuous Presence Multipoint (CPM) */
  putbits(1,0); /* CPM is not supported in this software */
  bits += 1;

  /* Picture Sub Bitstream Indicator (PSBI) */
  /* if CPM == 1: 2 bits PSBI */
  /* not supported */

  /* extra information for PB-frames */
  if (pic->PB) {
    if (trace) {
      fprintf(tf,"TRB: ");
    }
    putbits(3,pic->TRB);
    bits += 3;

    if (trace) {
      fprintf(tf,"BQUANT: ");
    }
    putbits(2,pic->BQUANT);
    bits += 2;
    
  }

  /* PEI (extra information) */
  if (trace) {
    fprintf(tf,"PEI: ");
  }
  /* "Encoders shall not insert PSPARE until specified by the ITU" */
  putbits(1,0); 
  bits += 1;

  /* PSPARE */
  /* if PEI == 1: 8 bits PSPARE + another PEI bit */
  /* not supported */

  return bits;
}

