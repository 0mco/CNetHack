/* NetHack 3.6	objects.c	$NHDT-Date: 1447313395 2015/11/12 07:29:55 $  $NHDT-Branch: master $:$NHDT-Revision: 1.49 $ */
/* Copyright (c) Mike Threepoint, 1989.                           */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * The data in this file is processed twice, to construct two arrays.
 * On the first pass, only object name and object description matter.
 * On the second pass, all object-class fields except those two matter.
 * 2nd pass is a recursive inclusion of this file, not a 2nd compilation.
 * The name/description array is also used by makedefs and lev_comp.
 *
 * #ifndef OBJECTS_PASS_2_
 * # define OBJECT(name,desc,foo,bar,glorkum) name,desc
 * struct objdescr obj_descr[] =
 * #else
 * # define OBJECT(name,desc,foo,bar,glorkum) foo,bar,glorkum
 * struct objclass objects[] =
 * #endif
 * {
 *   { OBJECT("strange object",NULL, 1,2,3) },
 *   { OBJECT("arrow","pointy stick", 4,5,6) },
 *   ...
 *   { OBJECT(NULL,NULL, 0,0,0) }
 * };
 * #define OBJECTS_PASS_2_
 * #include "objects.c"
 */

/* *INDENT-OFF* */
/* clang-format off */

#ifndef OBJECTS_PASS_2_
/* first pass */
struct monst { struct monst *dummy; };  /* lint: struct obj's union */
#include "config.h"
#include "obj.h"
#include "objclass.h"
#include "prop.h"
#include "skills.h"

#else /* !OBJECTS_PASS_2_ */
/* second pass */
#include "color.h"
#define COLOR_FIELD(X) X,
#endif /* !OBJECTS_PASS_2_ */

/* objects have symbols: ) [ = " ( % ! ? + / $ * ` 0 _ . */

/*
 *      Note:  OBJ() and BITS() macros are used to avoid exceeding argument
 *      limits imposed by some compilers.  The ctnr field of BITS currently
 *      does not map into struct objclass, and is ignored in the expansion.
 *      The 0 in the expansion corresponds to oc_pre_discovered, which is
 *      set at run-time during role-specific character initialization.
 */

#ifndef OBJECTS_PASS_2_
/* first pass -- object descriptive text */
#define OBJ(name,desc)  name, desc
#define OBJECT(obj,bits,prp,sym,prob,dly,wt, \
               cost,sdam,ldam,oc1,oc2,nut,color)  { obj }
#define None (char *) 0 /* less visual distraction for 'no description' */

NEARDATA struct objdescr obj_descr[] =
#else
/* second pass -- object definitions */
#define BITS(nmkn,mrg,uskn,ctnr,mgc,chrg,uniq,nwsh,big,tuf,dir,sub,mtrl) \
  nmkn,mrg,uskn,0,mgc,chrg,uniq,nwsh,big,tuf,dir,mtrl,sub /*SCO cpp fodder*/
#define OBJECT(obj,bits,prp,sym,prob,dly,wt,cost,sdam,ldam,oc1,oc2,nut,color) \
  { 0, 0, (char *) 0, bits, prp, sym, dly, COLOR_FIELD(color) prob, wt, \
    cost, sdam, ldam, oc1, oc2, nut }
#ifndef lint
#define HARDGEM(n) (n >= 8)
#else
#define HARDGEM(n) (0)
#endif

NEARDATA struct objclass objects[] =
#endif
{
/* dummy object[0] -- description [2nd arg] *must* be NULL */
OBJECT(OBJ("��ȕ���", None),
       BITS(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, P_NONE, 0),
       0, ILLOBJ_CLASS, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),

/* weapons ... */
#define WEAPON(name,desc,kn,mg,bi,prob,wt,                \
               cost,sdam,ldam,hitbon,typ,sub,metal,color) \
    OBJECT(OBJ(name,desc),                                          \
           BITS(kn, mg, 1, 0, 0, 1, 0, 0, bi, 0, typ, sub, metal),  \
           0, WEAPON_CLASS, prob, 0, wt,                            \
           cost, sdam, ldam, hitbon, 0, wt, color)
#define PROJECTILE(name,desc,kn,prob,wt,                  \
                   cost,sdam,ldam,hitbon,metal,sub,color) \
    OBJECT(OBJ(name,desc),                                          \
           BITS(kn, 1, 1, 0, 0, 1, 0, 0, 0, 0, PIERCE, sub, metal), \
           0, WEAPON_CLASS, prob, 0, wt,                            \
           cost, sdam, ldam, hitbon, 0, wt, color)
#define BOW(name,desc,kn,prob,wt,cost,hitbon,metal,sub,color) \
    OBJECT(OBJ(name,desc),                                          \
           BITS(kn, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, sub, metal),      \
           0, WEAPON_CLASS, prob, 0, wt,                            \
           cost, 2, 2, hitbon, 0, wt, color)

/* Note: for weapons that don't do an even die of damage (ex. 2-7 or 3-18)
   the extra damage is added on in weapon.c, not here! */

#define P PIERCE
#define S SLASH
#define B WHACK

/* missiles; materiel reflects the arrowhead, not the shaft */
PROJECTILE("��", None,
           1, 55, 1, 2, 6, 6, 0,        IRON, -P_BOW, HI_METAL),
PROJECTILE("�G���t�̖�", "�_��I�Ȗ�",
           0, 20, 1, 2, 7, 6, 0,        WOOD, -P_BOW, HI_WOOD),
PROJECTILE("�I�[�N�̖�", "�e���Ȗ�",
           0, 20, 1, 2, 5, 6, 0,        IRON, -P_BOW, CLR_BLACK),
PROJECTILE("��̖�", None,
           1, 12, 1, 5, 6, 6, 0,        SILVER, -P_BOW, HI_SILVER),
PROJECTILE("�|��", "�|�̖�",
           0, 15, 1, 4, 7, 7, 1,        METAL, -P_BOW, HI_METAL),
PROJECTILE("�N���X�{�D�{���g", None,
           1, 55, 1, 2, 4, 6, 0,        IRON, -P_CROSSBOW, HI_METAL),

/* missiles that don't use a launcher */
WEAPON("������", None,
       1, 1, 0, 60,   1,   2,  3,  2, 0, P,   -P_DART, IRON, HI_METAL),
WEAPON("�藠��", "���^�̓��������",
       0, 1, 0, 35,   1,   5,  8,  6, 2, P,   -P_SHURIKEN, IRON, HI_METAL),
WEAPON("�u�[������", None,
       1, 1, 0, 15,   5,  20,  9,  9, 0, 0,   -P_BOOMERANG, WOOD, HI_WOOD),

/* spears [note: javelin used to have a separate skill from spears,
   because the latter are primarily stabbing weapons rather than
   throwing ones; but for playability, they've been merged together
   under spear skill and spears can now be thrown like javelins] */
WEAPON("��", None,
       1, 1, 0, 50,  30,   3,  6,  8, 0, P,   P_SPEAR, IRON, HI_METAL),
WEAPON("�G���t�̑�", "�_��I�ȑ�",
       0, 1, 0, 10,  30,   3,  7,  8, 0, P,   P_SPEAR, WOOD, HI_WOOD),
WEAPON("�I�[�N�̑�", "�e���ȑ�",
       0, 1, 0, 13,  30,   3,  5,  8, 0, P,   P_SPEAR, IRON, CLR_BLACK),
WEAPON("�h���[�t�̑�", "��v�ȑ�",
       0, 1, 0, 12,  35,   3,  8,  8, 0, P,   P_SPEAR, IRON, HI_METAL),
WEAPON("��̑�", None,
       1, 1, 0,  2,  36,  40,  6,  8, 0, P,   P_SPEAR, SILVER, HI_SILVER),
WEAPON("�W���x����", "������",
       0, 1, 0, 10,  20,   3,  6,  6, 0, P,   P_SPEAR, IRON, HI_METAL),

/* spearish; doesn't stack, not intended to be thrown */
WEAPON("�g���C�f���g", None,
       1, 0, 0,  8,  25,   5,  6,  4, 0, P,   P_TRIDENT, IRON, HI_METAL),
        /* +1 small, +2d4 large */

/* blades; all stack */
WEAPON("�Z��", None,
       1, 1, 0, 30,  10,   4,  4,  3, 2, P,   P_DAGGER, IRON, HI_METAL),
WEAPON("�G���t�̒Z��", "�_��I�ȒZ��",
       0, 1, 0, 10,  10,   4,  5,  3, 2, P,   P_DAGGER, WOOD, HI_WOOD),
WEAPON("�I�[�N�̒Z��", "�e���ȒZ��",
       0, 1, 0, 12,  10,   4,  3,  3, 2, P,   P_DAGGER, IRON, CLR_BLACK),
WEAPON("��̒Z��", None,
       1, 1, 0,  3,  12,  40,  4,  3, 2, P,   P_DAGGER, SILVER, HI_SILVER),
WEAPON("�A�T��", None,
       1, 1, 0,  0,  10,   4,  4,  3, 2, S,   P_DAGGER, IRON, HI_METAL),
WEAPON("���X", None,
       1, 1, 0,  0,   5,   6,  3,  3, 2, S,   P_KNIFE, METAL, HI_METAL),
WEAPON("�i�C�t", None,
       1, 1, 0, 20,   5,   4,  3,  2, 0, P|S, P_KNIFE, IRON, HI_METAL),
WEAPON("�X�e�B���b�g", None,
       1, 1, 0,  5,   5,   4,  3,  2, 0, P|S, P_KNIFE, IRON, HI_METAL),
/* 3.6: worm teeth and crysknives now stack;
   when a stack of teeth is enchanted at once, they fuse into one crysknife;
   when a stack of crysknives drops, the whole stack reverts to teeth */
WEAPON("���[���̎�", None,
       1, 1, 0,  0,  20,   2,  2,  2, 0, 0,   P_KNIFE, 0, CLR_WHITE),
WEAPON("�N���X�i�C�t", None,
       1, 1, 0,  0,  20, 100, 10, 10, 3, P,   P_KNIFE, MINERAL, CLR_WHITE),

/* axes */
WEAPON("��", None,
       1, 0, 0, 40,  60,   8,  6,  4, 0, S,   P_AXE, IRON, HI_METAL),
WEAPON("�핀", "���n�̕�",       /* "double-bitted"? */
       0, 0, 1, 10, 120,  40,  8,  6, 0, S,   P_AXE, IRON, HI_METAL),

/* swords */
WEAPON("����", None,
       1, 0, 0,  8,  30,  10,  6,  8, 0, P,   P_SHORT_SWORD, IRON, HI_METAL),
WEAPON("�G���t�̏���", "�_��I�ȏ���",
       0, 0, 0,  2,  30,  10,  8,  8, 0, P,   P_SHORT_SWORD, WOOD, HI_WOOD),
WEAPON("�I�[�N�̏���", "�e���ȏ���",
       0, 0, 0,  3,  30,  10,  5,  8, 0, P,   P_SHORT_SWORD, IRON, CLR_BLACK),
WEAPON("�h���[�t�̏���", "���L�̏���",
       0, 0, 0,  2,  30,  10,  7,  8, 0, P,   P_SHORT_SWORD, IRON, HI_METAL),
WEAPON("�V�~�^�[", "�Ȃ�����",
       0, 0, 0, 15,  40,  15,  8,  8, 0, S,   P_SCIMITAR, IRON, HI_METAL),
WEAPON("��̃T�[�x��", None,
       1, 0, 0,  6,  40,  75,  8,  8, 0, S,   P_SABER, SILVER, HI_SILVER),
WEAPON("���L�̌�", None,
       1, 0, 0,  8,  70,  10,  4,  6, 0, S,   P_BROAD_SWORD, IRON, HI_METAL),
        /* +d4 small, +1 large */
WEAPON("�G���t�̕��L�̌�", "�_��I�ȕ��L�̌�",
       0, 0, 0,  4,  70,  10,  6,  6, 0, S,   P_BROAD_SWORD, WOOD, HI_WOOD),
        /* +d4 small, +1 large */
WEAPON("����", None,
       1, 0, 0, 50,  40,  15,  8, 12, 0, S,   P_LONG_SWORD, IRON, HI_METAL),
WEAPON("���茕", None,
       1, 0, 1, 22, 150,  50, 12,  6, 0, S,   P_TWO_HANDED_SWORD,
                                                            IRON, HI_METAL),
        /* +2d6 large */
WEAPON("��", "���̌�",
       0, 0, 0,  4,  40,  80, 10, 12, 1, S,   P_LONG_SWORD, IRON, HI_METAL),
/* special swords set up for artifacts */
WEAPON("�品", "���̒���",
       0, 0, 1,  0,  60, 500, 16,  8, 2, S,   P_TWO_HANDED_SWORD,
                                                            METAL, HI_METAL),
        /* +2d6 large */
WEAPON("���[���̌�", "�_��I�ȕ��L�̌�",
       0, 0, 0,  0,  40, 300,  4,  6, 0, S,   P_BROAD_SWORD, IRON, CLR_BLACK),
        /* +d4 small, +1 large; Stormbringer: +5d2 +d8 from level drain */

/* polearms */
/* spear-type */
WEAPON("�p���`�U��", "�e�G�Ȓ���",
       0, 0, 1,  5,  80,  10,  6,  6, 0, P,   P_POLEARMS, IRON, HI_METAL),
        /* +1 large */
WEAPON("�����T�[", "���t�̒���",
       0, 0, 1,  5,  50,   6,  4,  4, 0, P,   P_POLEARMS, IRON, HI_METAL),
        /* +d4 both */
WEAPON("�X�y�^��", "�t�H�[�N�t������",
       0, 0, 1,  5,  50,   5,  6,  6, 0, P,   P_POLEARMS, IRON, HI_METAL),
        /* +1 small, +d6 large */
WEAPON("�O���C�u", "�Аn����",
       0, 0, 1,  8,  75,   6,  6, 10, 0, S,   P_POLEARMS, IRON, HI_METAL),
WEAPON("�����X", None,
       1, 0, 0,  4, 180,  10,  6,  8, 0, P,   P_LANCE, IRON, HI_METAL),
        /* +2d10 when jousting with lance as primary weapon */
/* axe-type */
WEAPON("�n���o�[�h", "�Ȃ����܂�����",
       0, 0, 1,  8, 150,  10, 10,  6, 0, P|S, P_POLEARMS, IRON, HI_METAL),
        /* +1d6 large */
WEAPON("�o�[�f�B�b�N", "�����܂�����",
       0, 0, 1,  4, 120,   7,  4,  4, 0, S,   P_POLEARMS, IRON, HI_METAL),
        /* +1d4 small, +2d4 large */
WEAPON("���H�E�W�F", "��t����",
       0, 0, 1,  4, 125,   5,  4,  4, 0, S,   P_POLEARMS, IRON, HI_METAL),
        /* +d4 both */
WEAPON("�h���[�t�̂�͂�", "���L�̂�͂�",
       0, 0, 1, 13, 120,  50, 12,  8, -1, B,  P_PICK_AXE, IRON, HI_METAL),
/* curved/hooked */
WEAPON("�t�H�V���[��", "���t����",
       0, 0, 1,  6,  60,   5,  6,  8, 0, P|S, P_POLEARMS, IRON, HI_METAL),
WEAPON("�M�U����", "���荞�݂���",
       0, 0, 1,  6,  80,   5,  4,  8, 0, S,   P_POLEARMS, IRON, HI_METAL),
        /* +1d4 small */
WEAPON("�r���E�M�U����", "�b�t������",
       0, 0, 1,  4, 120,   7,  4, 10, 0, P|S, P_POLEARMS, IRON, HI_METAL),
        /* +1d4 small */
/* other */
WEAPON("���b�c�F�����n���}�[", "��҂̒���",
       0, 0, 1,  5, 150,   7,  4,  6, 0, B|P, P_POLEARMS, IRON, HI_METAL),
        /* +1d4 small */
WEAPON("�x�b�N�E�h�E�R���o��", "�����΂��t������",
       0, 0, 1,  4, 100,   8,  8,  6, 0, B|P, P_POLEARMS, IRON, HI_METAL),

/* bludgeons */
WEAPON("���C�X", None,
       1, 0, 0, 40,  30,   5,  6,  6, 0, B,   P_MACE, IRON, HI_METAL),
        /* +1 small */
WEAPON("���[�j���O�X�^�[", None,
       1, 0, 0, 12, 120,  10,  4,  6, 0, B,   P_MORNING_STAR, IRON, HI_METAL),
        /* +d4 small, +1 large */
WEAPON("�E�H�[�n���}�[", None,
       1, 0, 0, 15,  50,   5,  4,  4, 0, B,   P_HAMMER, IRON, HI_METAL),
        /* +1 small */
WEAPON("����_", None,
       1, 0, 0, 12,  30,   3,  6,  3, 0, B,   P_CLUB, WOOD, HI_WOOD),
WEAPON("�S���z�[�X", None,
       1, 0, 0,  0,  20,   3,  4,  3, 0, B,   P_WHIP, PLASTIC, CLR_BROWN),
WEAPON("�Z�ږ_", "�_",
       0, 0, 1, 11,  40,   5,  6,  6, 0, B,   P_QUARTERSTAFF, WOOD, HI_WOOD),
/* two-piece */
WEAPON("�A�L���X", "�R�t�̂���_",
       0, 0, 0,  8,  15,   4,  6,  3, 0, B,   P_CLUB, IRON, HI_METAL),
WEAPON("�t���C��", None,
       1, 0, 0, 40,  15,   4,  6,  4, 0, B,   P_FLAIL, IRON, HI_METAL),
        /* +1 small, +1d4 large */

/* misc */
WEAPON("��", None,
       1, 0, 0,  2,  20,   4,  2,  1, 0, 0,   P_WHIP, LEATHER, CLR_BROWN),

/* bows */
BOW("�|", None,                 1, 24, 30, 60, 0, WOOD, P_BOW, HI_WOOD),
BOW("�G���t�̋|", "�_��I�ȋ|", 0, 12, 30, 60, 0, WOOD, P_BOW, HI_WOOD),
BOW("�I�[�N�̋|", "�e���ȋ|",   0, 12, 30, 60, 0, WOOD, P_BOW, CLR_BLACK),
BOW("�a�|", "���|",             0,  0, 30, 60, 0, WOOD, P_BOW, HI_WOOD),
BOW("�X�����O", None,           1, 40,  3, 20, 0, LEATHER, P_SLING, HI_LEATHER),
BOW("�N���X�{�D", None,         1, 45, 50, 40, 0, WOOD, P_CROSSBOW, HI_WOOD),

#undef P
#undef S
#undef B

#undef WEAPON
#undef PROJECTILE
#undef BOW

/* armor ... */
        /* IRON denotes ferrous metals, including steel.
         * Only IRON weapons and armor can rust.
         * Only COPPER (including brass) corrodes.
         * Some creatures are vulnerable to SILVER.
         */
#define ARMOR(name,desc,kn,mgc,blk,power,prob,delay,wt,  \
              cost,ac,can,sub,metal,c)                   \
    OBJECT(OBJ(name, desc),                                         \
           BITS(kn, 0, 1, 0, mgc, 1, 0, 0, blk, 0, 0, sub, metal),  \
           power, ARMOR_CLASS, prob, delay, wt,                     \
           cost, 0, 0, 10 - ac, can, wt, c)
#define HELM(name,desc,kn,mgc,power,prob,delay,wt,cost,ac,can,metal,c)  \
    ARMOR(name, desc, kn, mgc, 0, power, prob, delay, wt,  \
          cost, ac, can, ARM_HELM, metal, c)
#define CLOAK(name,desc,kn,mgc,power,prob,delay,wt,cost,ac,can,metal,c)  \
    ARMOR(name, desc, kn, mgc, 0, power, prob, delay, wt,  \
          cost, ac, can, ARM_CLOAK, metal, c)
#define SHIELD(name,desc,kn,mgc,blk,power,prob,delay,wt,cost,ac,can,metal,c) \
    ARMOR(name, desc, kn, mgc, blk, power, prob, delay, wt, \
          cost, ac, can, ARM_SHIELD, metal, c)
#define GLOVES(name,desc,kn,mgc,power,prob,delay,wt,cost,ac,can,metal,c)  \
    ARMOR(name, desc, kn, mgc, 0, power, prob, delay, wt,  \
          cost, ac, can, ARM_GLOVES, metal, c)
#define BOOTS(name,desc,kn,mgc,power,prob,delay,wt,cost,ac,can,metal,c)  \
    ARMOR(name, desc, kn, mgc, 0, power, prob, delay, wt,  \
          cost, ac, can, ARM_BOOTS, metal, c)

/* helmets */
HELM("�G���t�̊v�X�q", "�v�X�q",
     0, 0,           0,  6, 1,  3,  8,  9, 0, LEATHER, HI_LEATHER),
HELM("�I�[�N�̊�", "�S�̖X�q",
     0, 0,           0,  6, 1, 30, 10,  9, 0, IRON, CLR_BLACK),
HELM("�h���[�t�̓S��", "�ł��X�q",
     0, 0,           0,  6, 1, 40, 20,  8, 0, IRON, HI_METAL),
HELM("�t�B�h�[��", None,
     1, 0,           0,  0, 0,  3,  1, 10, 0, CLOTH, CLR_BROWN),
HELM("�Ƃ񂪂�X�q", "�~���`�̖X�q",
     0, 1, CLAIRVOYANT,  3, 1,  4, 80, 10, 1, CLOTH, CLR_BLUE),
        /* name coined by devteam; confers clairvoyance for wizards,
           blocks clairvoyance if worn by role other than wizard */
HELM("��\�X", "�~���`�̖X�q",
     0, 1,           0,  3, 1,  4,  1, 10, 0, CLOTH, CLR_BLUE),
HELM("���ڂ񂾓�", None,
     1, 0,           0,  2, 0, 10,  8,  9, 0, IRON, CLR_BLACK),
/* with shuffled appearances... */
HELM("��", "�H��",
     0, 0,           0, 10, 1, 30, 10,  9, 0, IRON, HI_METAL),
HELM("�m���̊�", "�͗l���芕",
     0, 1,           0,  6, 1, 50, 50,  9, 0, IRON, CLR_GREEN),
HELM("�t�����̊�", "�Ƃ����̊�",
     0, 1,           0,  6, 1, 50, 50,  9, 0, IRON, HI_METAL),
HELM("�e���p�V�[�̊�", "�ʖj�t���̊�",
     0, 1,     TELEPAT,  2, 1, 50, 50,  9, 0, IRON, HI_METAL),

/* suits of armor */
/*
 * There is code in polyself.c that assumes (1) and (2).
 * There is code in obj.h, objnam.c, mon.c, read.c that assumes (2).
 *      (1) The dragon scale mails and the dragon scales are together.
 *      (2) That the order of the dragon scale mail and dragon scales
 *          is the the same as order of dragons defined in monst.c.
 */
#define DRGN_ARMR(name,mgc,power,cost,ac,color)  \
    ARMOR(name, None, 1, mgc, 1, power, 0, 5, 40,  \
          cost, ac, 0, ARM_SUIT, DRAGON_HIDE, color)
/* 3.4.1: dragon scale mail reclassified as "magic" since magic is
   needed to create them */
DRGN_ARMR("�D�F�h���S���̗؊Z",     1, ANTIMAGIC,  1200, 1, CLR_GRAY),
DRGN_ARMR("��F�h���S���̗؊Z",     1, REFLECTING, 1200, 1, DRAGON_SILVER),
#if 0 /* DEFERRED */
DRGN_ARMR("shimmering dragon scale mail", 1, DISPLACED, 1200, 1, CLR_CYAN),
#endif
DRGN_ARMR("�ԐF�h���S���̗؊Z",     1, FIRE_RES,    900, 1, CLR_RED),
DRGN_ARMR("���h���S���̗؊Z",       1, COLD_RES,    900, 1, CLR_WHITE),
DRGN_ARMR("�I�����W�h���S���̗؊Z", 1, SLEEP_RES,   900, 1, CLR_ORANGE),
DRGN_ARMR("���h���S���̗؊Z",       1, DISINT_RES, 1200, 1, CLR_BLACK),
DRGN_ARMR("�h���S���̗؊Z",       1, SHOCK_RES,   900, 1, CLR_BLUE),
DRGN_ARMR("�΃h���S���̗؊Z",       1, POISON_RES,  900, 1, CLR_GREEN),
DRGN_ARMR("���F�h���S���̗؊Z",     1, ACID_RES,    900, 1, CLR_YELLOW),
/* For now, only dragons leave these. */
/* 3.4.1: dragon scales left classified as "non-magic"; they confer
   magical properties but are produced "naturally" */
DRGN_ARMR("�D�F�h���S���̗�",       0, ANTIMAGIC,   700, 7, CLR_GRAY),
DRGN_ARMR("��F�h���S���̗�",       0, REFLECTING,  700, 7, DRAGON_SILVER),
#if 0 /* DEFERRED */
DRGN_ARMR("shimmering dragon scales",  0, DISPLACED,   700, 7, CLR_CYAN),
#endif
DRGN_ARMR("�ԐF�h���S���̗�",       0, FIRE_RES,    500, 7, CLR_RED),
DRGN_ARMR("���h���S���̗�",         0, COLD_RES,    500, 7, CLR_WHITE),
DRGN_ARMR("�I�����W�h���S���̗�",   0, SLEEP_RES,   500, 7, CLR_ORANGE),
DRGN_ARMR("���h���S���̗�",         0, DISINT_RES,  700, 7, CLR_BLACK),
DRGN_ARMR("�h���S���̗�",         0, SHOCK_RES,   500, 7, CLR_BLUE),
DRGN_ARMR("�΃h���S���̗�",         0, POISON_RES,  500, 7, CLR_GREEN),
DRGN_ARMR("���F�h���S���̗�",       0, ACID_RES,    500, 7, CLR_YELLOW),
#undef DRGN_ARMR
/* other suits */
ARMOR("�|�S�̊Z", None,
      1, 0, 1,  0, 44, 5, 450, 600,  3, 2,  ARM_SUIT, IRON, HI_METAL),
ARMOR("�����̊Z", None,
      1, 0, 1,  0, 10, 5, 450, 820,  3, 2,  ARM_SUIT, GLASS, CLR_WHITE),
ARMOR("���̊Z", None,
      1, 0, 1,  0, 25, 5, 450, 400,  4, 1,  ARM_SUIT, COPPER, HI_COPPER),
ARMOR("�S�Ђ̊Z", None,
      1, 0, 1,  0, 62, 5, 400,  80,  4, 1,  ARM_SUIT, IRON, HI_METAL),
ARMOR("�ы��̊Z", None,
      1, 0, 1,  0, 72, 5, 350,  90,  4, 1,  ARM_SUIT, IRON, HI_METAL),
ARMOR("�h���[�t�̃~�X������", None,
      1, 0, 0,  0, 10, 1, 150, 240,  4, 2,  ARM_SUIT, MITHRIL, HI_SILVER),
ARMOR("�G���t�̃~�X������", None,
      1, 0, 0,  0, 15, 1, 150, 240,  5, 2,  ARM_SUIT, MITHRIL, HI_SILVER),
ARMOR("�������т�", None,
      1, 0, 0,  0, 72, 5, 300,  75,  5, 1,  ARM_SUIT, IRON, HI_METAL),
ARMOR("�I�[�N�̍������т�", "�e���ȍ������т�",
      0, 0, 0,  0, 20, 5, 300,  75,  6, 1,  ARM_SUIT, IRON, CLR_BLACK),
ARMOR("�؂̊Z", None,
      1, 0, 0,  0, 72, 5, 250,  45,  6, 1,  ARM_SUIT, IRON, HI_METAL),
ARMOR("�e�t���v�Z", None,
      1, 0, 0,  0, 72, 3, 200,  15,  7, 1,  ARM_SUIT, LEATHER, HI_LEATHER),
ARMOR("�S�̊Z", None,
      1, 0, 0,  0, 72, 5, 250, 100,  7, 1,  ARM_SUIT, IRON, HI_METAL),
ARMOR("�I�[�N�̓S�̊Z", "�e���ȓS�̊Z",
      0, 0, 0,  0, 20, 5, 250,  80,  8, 1,  ARM_SUIT, IRON, CLR_BLACK),
ARMOR("�v�Z", None,
      1, 0, 0,  0, 82, 3, 150,   5,  8, 1,  ARM_SUIT, LEATHER, HI_LEATHER),
ARMOR("�v�̕�", None,
      1, 0, 0,  0, 12, 0,  30,  10,  9, 0,  ARM_SUIT, LEATHER, CLR_BLACK),

/* shirts */
ARMOR("�A���n�V���c", None,
      1, 0, 0,  0,  8, 0,   5,   3, 10, 0,  ARM_SHIRT, CLOTH, CLR_MAGENTA),
ARMOR("�s�V���c", None,
      1, 0, 0,  0,  2, 0,   5,   2, 10, 0,  ARM_SHIRT, CLOTH, CLR_WHITE),

/* cloaks */
CLOAK("�~�C���̕��", None,
      1, 0,          0,  0, 0,  3,  2, 10, 1,  CLOTH, CLR_GRAY),
        /* worn mummy wrapping blocks invisibility */
CLOAK("�G���t�̃N���[�N", "�A�C�ȊO��",
      0, 1,    STEALTH,  8, 0, 10, 60,  9, 1,  CLOTH, CLR_BLACK),
CLOAK("�I�[�N�̃N���[�N", "�e���ȃ}���g",
      0, 0,          0,  8, 0, 10, 40, 10, 1,  CLOTH, CLR_BLACK),
CLOAK("�h���[�t�̃N���[�N", "�t�[�h���̃N���[�N",
      0, 0,          0,  8, 0, 10, 50, 10, 1,  CLOTH, HI_CLOTH),
CLOAK("�h���N���[�N", "��邵���N���[�N",
      0, 0,          0,  8, 0, 10, 50,  9, 2,  CLOTH, HI_CLOTH),
CLOAK("���[�u", None,
      1, 1,          0,  3, 0, 15, 50,  8, 2,  CLOTH, CLR_RED),
        /* robe was adopted from slash'em, where it's worn as a suit
           rather than as a cloak and there are several variations */
CLOAK("�B���p�̎d����", "�G�v����",
      0, 1, POISON_RES,  9, 0, 10, 50,  9, 1,  CLOTH, CLR_WHITE),
CLOAK("�v�̃N���[�N", None,
      1, 0,          0,  8, 0, 15, 40,  9, 1,  LEATHER, CLR_BROWN),
/* with shuffled appearances... */
CLOAK("���̃N���[�N", "�ڂ�ڂ�̃P�[�v",
      0, 1, PROTECTION,  9, 0, 10, 50,  7, 3,  CLOTH, HI_CLOTH),
        /* cloak of protection is now the only item conferring MC 3 */
CLOAK("�����̃N���[�N", "�I�y���N���[�N",
      0, 1,      INVIS, 10, 0, 10, 60,  9, 1,  CLOTH, CLR_BRIGHT_MAGENTA),
CLOAK("���@��h���N���[�N", "�����p�̊O��",
      0, 1,  ANTIMAGIC,  2, 0, 10, 60,  9, 1,  CLOTH, CLR_WHITE),
        /*  'cope' is not a spelling mistake... leave it be */
CLOAK("���e�̃N���[�N", "�z�؂�",
      0, 1,  DISPLACED, 10, 0, 10, 50,  9, 1,  CLOTH, HI_CLOTH),

/* shields */
SHIELD("�����ȏ�", None,
       1, 0, 0,          0, 6, 0,  30,  3, 9, 0,  WOOD, HI_WOOD),
SHIELD("�G���t�̏�", "�Ɨ΂̏�",
       0, 0, 0,          0, 2, 0,  40,  7, 8, 0,  WOOD, CLR_GREEN),
SHIELD("�E���N�E�n�C�̏�", "���̎�̏�",
       0, 0, 0,          0, 2, 0,  50,  7, 9, 0,  IRON, HI_METAL),
SHIELD("�I�[�N�̏�", "�Ԃ��ڂ̏�",
       0, 0, 0,          0, 2, 0,  50,  7, 9, 0,  IRON, CLR_RED),
SHIELD("�傫�ȏ�", None,
       1, 0, 1,          0, 7, 0, 100, 10, 8, 0,  IRON, HI_METAL),
SHIELD("�h���[�t�̊ۏ�", "�傫�Ȋۏ�",
       0, 0, 0,          0, 4, 0, 100, 10, 8, 0,  IRON, HI_METAL),
SHIELD("���˂̏�", "��F�̖����ꂽ��",
       0, 1, 0, REFLECTING, 3, 0,  50, 50, 8, 0,  SILVER, HI_SILVER),

/* gloves */
/* These have their color but not material shuffled, so the IRON must
 * stay CLR_BROWN (== HI_LEATHER) even though it's normally either
 * HI_METAL or CLR_BLACK.  All have shuffled descriptions.
 */
GLOVES("�v�̎��", "�Â����",
       0, 0,        0, 16, 1, 10,  8, 9, 0,  LEATHER, HI_LEATHER),
GLOVES("����ʂ̏���", "�l�߂��̂̂�����",
       0, 1, FUMBLING,  8, 1, 10, 50, 9, 0,  LEATHER, HI_LEATHER),
GLOVES("�͂̏���", "��n�p�̎��",
       0, 1,        0,  8, 1, 30, 50, 9, 0,  IRON, CLR_BROWN),
GLOVES("��p���̏���", "�t�F���V���O�̏���",
       0, 1,        0,  8, 1, 10, 50, 9, 0,  LEATHER, HI_LEATHER),

/* boots */
BOOTS("�����Ƃ̒Ⴂ�C", "�U���p�̌C",
      0, 0,          0, 25, 2, 10,  8, 9, 0, LEATHER, HI_LEATHER),
BOOTS("�S�̌C", "�ł��C",
      0, 0,          0,  7, 2, 50, 16, 8, 0, IRON, HI_METAL),
BOOTS("�����Ƃ̍����C", "�R���C",
      0, 0,          0, 15, 2, 20, 12, 8, 0, LEATHER, HI_LEATHER),
/* with shuffled appearances... */
BOOTS("��ʓV�̌C", "�퓬�C",
      0, 1,       FAST, 12, 2, 20, 50, 9, 0, LEATHER, HI_LEATHER),
BOOTS("������s�̌C", "�W�����O���̌C",
      0, 1,   WWALKING, 12, 2, 15, 50, 9, 0, LEATHER, HI_LEATHER),
BOOTS("��ђ��˂�C", "�n�C�L���O�̌C",
      0, 1,    JUMPING, 12, 2, 20, 50, 9, 0, LEATHER, HI_LEATHER),
BOOTS("�G���t�̌C", "���C",
      0, 1,    STEALTH, 12, 2, 15,  8, 9, 0, LEATHER, HI_LEATHER),
BOOTS("�R�苓���C", "���ߋ��̂���C",
      0, 1,          0, 12, 2, 50,  8, 9, 0, IRON, CLR_BROWN),
        /* CLR_BROWN for same reason as gauntlets of power */
BOOTS("�܂����̌C", "��n�p�̌C",
      0, 1,   FUMBLING, 12, 2, 20, 30, 9, 0, LEATHER, HI_LEATHER),
BOOTS("���V�̌C", "��C",
      0, 1, LEVITATION, 12, 2, 15, 30, 9, 0, LEATHER, HI_LEATHER),
#undef HELM
#undef CLOAK
#undef SHIELD
#undef GLOVES
#undef BOOTS
#undef ARMOR

/* rings ... */
#define RING(name,stone,power,cost,mgc,spec,mohs,metal,color) \
    OBJECT(OBJ(name, stone),                                          \
           BITS(0, 0, spec, 0, mgc, spec, 0, 0, 0,                    \
                HARDGEM(mohs), 0, P_NONE, metal),                     \
           power, RING_CLASS, 0, 0, 3, cost, 0, 0, 0, 0, 15, color)
RING("����̎w��", "�؂̎w��",
     ADORNED,                  100, 1, 1, 2, WOOD, HI_WOOD),
RING("�����̎w��", "�ԛ���̎w��",
     0,                        150, 1, 1, 7, MINERAL, HI_MINERAL),
RING("�̗͂̎w��", "�I�p�[���̎w��",
     0,                        150, 1, 1, 7, MINERAL, HI_MINERAL),
RING("�����̎w��", "�y�̎w��",
     0,                        150, 1, 1, 4, MINERAL, CLR_RED),
RING("�U���̎w��", "�X��̎w��",
     0,                        150, 1, 1, 4, MINERAL, CLR_ORANGE),
RING("���̎w��", "���߂̂��̎w��",
     PROTECTION,               100, 1, 1, 7, MINERAL, CLR_BLACK),
        /* 'PROTECTION' intrinsic enhances MC from worn armor by +1,
           regardless of ring's enchantment; wearing a second ring of
           protection (or even one ring of protection combined with
           cloak of protection) doesn't give a second MC boost */
RING("�񕜂̎w��", "�����΂̎w��",
     REGENERATION,             200, 1, 0,  6, MINERAL, HI_MINERAL),
RING("�T���̎w��", "�Ֆڐ΂̎w��",
     SEARCHING,                200, 1, 0,  6, GEMSTONE, CLR_BROWN),
RING("�E�т̎w��", "�Ђ����̎w��",
     STEALTH,                  100, 1, 0,  6, GEMSTONE, CLR_GREEN),
RING("�\�͈ێ��̎w��", "���̎w��",
     FIXED_ABIL,               100, 1, 0,  4, COPPER, HI_COPPER),
RING("���V�̎w��", "�߂̂��̎w��",
     LEVITATION,               200, 1, 0,  7, GEMSTONE, CLR_RED),
RING("�Q��̎w��", "�g�p�[�Y�̎w��",
     HUNGER,                   100, 1, 0,  8, GEMSTONE, CLR_CYAN),
RING("�����̎w��", "�T�t�@�C�A�̎w��",
     AGGRAVATE_MONSTER,        150, 1, 0,  9, GEMSTONE, CLR_BLUE),
RING("�����̎w��", "���r�[�̎w��",
     CONFLICT,                 300, 1, 0,  9, GEMSTONE, CLR_RED),
RING("�x���̎w��", "�_�C�������h�̎w��",
     WARNING,                  100, 1, 0, 10, GEMSTONE, CLR_WHITE),
RING("�ϓł̎w��", "�^��̎w��",
     POISON_RES,               150, 1, 0,  4, BONE, CLR_WHITE),
RING("�ω��̎w��", "�S�̎w��",
     FIRE_RES,                 200, 1, 0,  5, IRON, HI_METAL),
RING("�ϗ�̎w��", "�^�J�̎w��",
     COLD_RES,                 150, 1, 0,  4, COPPER, HI_COPPER),
RING("�ϓd�̎w��", "���̎w��",
     SHOCK_RES,                150, 1, 0,  3, COPPER, HI_COPPER),
RING("���R�s���̎w��", "�˂��ꂽ�w��",
     FREE_ACTION,              200, 1, 0,  6, IRON, HI_METAL),
RING("�����s�ǂ̎w��", "�|�S�̎w��",
     SLOW_DIGESTION,           200, 1, 0,  8, IRON, HI_METAL),
RING("�u�Ԉړ��̎w��", "��̎w��",
     TELEPORT,                 200, 1, 0,  3, SILVER, HI_SILVER),
RING("�u�Ԉړ�����̎w��", "���̎w��",
     TELEPORT_CONTROL,         300, 1, 0,  3, GOLD, HI_GOLD),
RING("�ω��̎w��", "�ۉ�̎w��",
     POLYMORPH,                300, 1, 0,  4, BONE, CLR_WHITE),
RING("�ω�����̎w��", "�G�������h�̎w��",
     POLYMORPH_CONTROL,        300, 1, 0,  8, GEMSTONE, CLR_BRIGHT_GREEN),
RING("�����̎w��", "�j���̎w��",
     INVIS,                    150, 1, 0,  5, IRON, HI_METAL),
RING("���̎w��", "����w��",
     SEE_INVIS,                150, 1, 0,  5, IRON, HI_METAL),
RING("�ϕω������̎w��", "����w��",
     PROT_FROM_SHAPE_CHANGERS, 100, 1, 0,  5, IRON, CLR_BRIGHT_CYAN),
#undef RING

/* amulets ... - THE Amulet comes last because it is special */
#define AMULET(name,desc,power,prob) \
    OBJECT(OBJ(name, desc),                                            \
           BITS(0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, P_NONE, IRON),        \
           power, AMULET_CLASS, prob, 0, 20, 150, 0, 0, 0, 0, 20, HI_METAL)
AMULET("�����̖�����",     "�~�`�̖�����", TELEPAT, 175),
AMULET("���̖�����",       "���`�̖�����", LIFESAVED, 75),
AMULET("�i�E�̖�����",     "���^�̖�����", STRANGLED, 135),
AMULET("�����̖�����",   "�O�p�`�̖�����", SLEEPY, 135),
AMULET("�ϓł̖�����",   "�l�p���̖�����", POISON_RES, 165),
AMULET("���]���̖�����",   "�l�p�̖�����", 0, 130),
AMULET("���ω��̖�����",   "���ʂ̖�����", UNCHANGING, 45),
AMULET("���˂̖�����",   "�Z�p�`�̖�����", REFLECTING, 75),
AMULET("�ċz�̖�����",   "���p�`�̖�����", MAGICAL_BREATHING, 65),
/* fixed descriptions; description duplication is deliberate;
 * fake one must come before real one because selection for
 * description shuffling stops when a non-magic amulet is encountered
 */
OBJECT(OBJ("�U���̃C�F���_�[�̖�����",
           "�C�F���_�[�̖�����"),
       BITS(0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, PLASTIC),
       0, AMULET_CLASS, 0, 0, 20, 0, 0, 0, 0, 0, 1, HI_METAL),
OBJECT(OBJ("�C�F���_�[�̖�����", /* note: description == name */
           "�C�F���_�[�̖�����"),
       BITS(0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, MITHRIL),
       0, AMULET_CLASS, 0, 0, 20, 30000, 0, 0, 0, 0, 20, HI_METAL),
#undef AMULET

/* tools ... */
/* tools with weapon characteristics come last */
#define TOOL(name,desc,kn,mrg,mgc,chg,prob,wt,cost,mat,color) \
    OBJECT(OBJ(name, desc),                                             \
           BITS(kn, mrg, chg, 0, mgc, chg, 0, 0, 0, 0, 0, P_NONE, mat), \
           0, TOOL_CLASS, prob, 0, wt, cost, 0, 0, 0, 0, wt, color)
#define CONTAINER(name,desc,kn,mgc,chg,prob,wt,cost,mat,color) \
    OBJECT(OBJ(name, desc),                                             \
           BITS(kn, 0, chg, 1, mgc, chg, 0, 0, 0, 0, 0, P_NONE, mat),   \
           0, TOOL_CLASS, prob, 0, wt, cost, 0, 0, 0, 0, wt, color)
#define WEPTOOL(name,desc,kn,mgc,bi,prob,wt,cost,sdam,ldam,hitbon,sub,mat,clr)\
    OBJECT(OBJ(name, desc),                                             \
           BITS(kn, 0, 1, 0, mgc, 1, 0, 0, bi, 0, hitbon, sub, mat),    \
           0, TOOL_CLASS, prob, 0, wt, cost, sdam, ldam, hitbon, 0, wt, clr)
/* containers */
CONTAINER("�唠",                    None, 1, 0, 0, 40, 350,   8, WOOD, HI_WOOD),
CONTAINER("��",                    None, 1, 0, 0, 35, 600,  16, WOOD, HI_WOOD),
CONTAINER("�A�C�X�{�b�N�X",          None, 1, 0, 0,  5, 900,  42, PLASTIC, CLR_WHITE),
CONTAINER("��",                      "��", 0, 0, 0, 35,  15,   2, CLOTH, HI_CLOTH),
CONTAINER("�h����",                  "��", 0, 0, 0,  5,  15, 100, CLOTH, HI_CLOTH),
CONTAINER("�y�ʉ��̊�",              "��", 0, 1, 0, 20,  15, 100, CLOTH, HI_CLOTH),
CONTAINER("�g���b�N�̊�",            "��", 0, 1, 1, 20,  15, 100, CLOTH, HI_CLOTH),
#undef CONTAINER

/* lock opening tools */
TOOL("���\��",                       "��", 0, 0, 0, 0, 80,  3, 10, IRON, HI_METAL),
TOOL("���J�����",                   None, 1, 0, 0, 0, 60,  4, 20, IRON, HI_METAL),
TOOL("�N���W�b�g�J�[�h",             None, 1, 0, 0, 0, 15,  1, 10, PLASTIC, CLR_WHITE),
/* light sources */
TOOL("�b���̂낤����",         "�낤����", 0, 1, 0, 0, 20,  2, 10, WAX, CLR_WHITE),
TOOL("���X�̂낤����",         "�낤����", 0, 1, 0, 0,  5,  2, 20, WAX, CLR_WHITE),
TOOL("�^�J�̃����^��",               None, 1, 0, 0, 0, 30, 30, 12, COPPER, CLR_YELLOW),
TOOL("�I�C�������v",             "�����v", 0, 0, 0, 0, 45, 20, 10, COPPER, CLR_YELLOW),
TOOL("���@�̃����v",             "�����v", 0, 0, 1, 0, 15, 20, 50, COPPER, CLR_YELLOW),
/* other tools */
TOOL("�����ȃJ����",                 None, 1, 0, 0, 1, 15, 12,200, PLASTIC, CLR_BLACK),
TOOL("��",                       "�K���X", 0, 0, 0, 0, 45, 13, 10, GLASS, HI_SILVER),
TOOL("������",               "�K���X�̋�", 0, 0, 1, 1, 15,150, 60, GLASS, HI_GLASS),
TOOL("�����Y",                       None, 1, 0, 0, 0,  5,  3, 80, GLASS, HI_GLASS),
TOOL("�ډB��",                       None, 1, 0, 0, 0, 50,  2, 20, CLOTH, CLR_BLACK),
TOOL("�^�I��",                       None, 1, 0, 0, 0, 50,  2, 50, CLOTH, CLR_MAGENTA),
TOOL("��",                           None, 1, 0, 0, 0,  5,200,150, LEATHER, HI_LEATHER),
TOOL("�R",                           None, 1, 0, 0, 0, 65, 12, 20, LEATHER, HI_LEATHER),
TOOL("���f��",                       None, 1, 0, 0, 0, 25,  4, 75, IRON, HI_METAL),
TOOL("�ʋl�쐬����",                 None, 1, 0, 0, 1, 15,100, 30, IRON, HI_METAL),
TOOL("�ʐ؂�",                       None, 1, 0, 0, 0, 35,  4, 30, IRON, HI_METAL),
TOOL("���̊�",                       None, 1, 0, 0, 1, 15, 15, 20, IRON, HI_METAL),
TOOL("�l�`",                         None, 1, 0, 1, 0, 25, 50, 80, MINERAL, HI_MINERAL),
        /* monster type specified by obj->corpsenm */
TOOL("���@�̃}�[�J",                 None, 1, 0, 1, 1, 15,  2, 50, PLASTIC, CLR_RED),
/* traps */
TOOL("�n��",                         None, 1, 0, 0, 0, 0, 300,180, IRON, CLR_RED),
TOOL("�F���",                       None, 1, 0, 0, 0, 0, 200, 60, IRON, HI_METAL),
/* instruments;
   "If tin whistles are made out of tin, what do they make foghorns out of?" */
TOOL("�u���L�̓J",                   "�J", 0, 0, 0, 0,100, 3, 10, METAL, HI_METAL),
TOOL("���@�̓J",                     "�J", 0, 0, 1, 0, 30, 3, 10, METAL, HI_METAL),
TOOL("�؂̃t���[�g",           "�t���[�g", 0, 0, 0, 0,  4, 5, 12, WOOD, HI_WOOD),
TOOL("���@�̃t���[�g",         "�t���[�g", 0, 0, 1, 1,  2, 5, 36, WOOD, HI_WOOD),
TOOL("�׍H�̂قǂ����ꂽ�z����", "�z����", 0, 0, 0, 0,  5, 18, 15, BONE, CLR_WHITE),
TOOL("����̃z����",             "�z����", 0, 0, 1, 1,  2, 18, 50, BONE, CLR_WHITE),
TOOL("���̃z����",               "�z����", 0, 0, 1, 1,  2, 18, 50, BONE, CLR_WHITE),
TOOL("�b�݂̃z����",             "�z����", 0, 0, 1, 1,  2, 18, 50, BONE, CLR_WHITE),
        /* horn, but not an instrument */
TOOL("�؂̒G��",                   "�G��", 0, 0, 0, 0,  4, 30, 50, WOOD, HI_WOOD),
TOOL("���@�̒G��",                 "�G��", 0, 0, 1, 1,  2, 30, 50, WOOD, HI_WOOD),
TOOL("�x��",                         None, 1, 0, 0, 0,  2, 30, 50, COPPER, HI_COPPER),
TOOL("���b�p",                       None, 1, 0, 0, 0,  4, 10, 15, COPPER, HI_COPPER),
TOOL("�v�̑���",                   "����", 0, 0, 0, 0,  4, 25, 25, LEATHER, HI_LEATHER),
TOOL("�n�k�̑���",                 "����", 0, 0, 1, 1,  2, 25, 25, LEATHER, HI_LEATHER),
/* tools useful as weapons */
WEPTOOL("��͂�", None,
        1, 0, 0, 20, 100,  50,  6,  3, WHACK,  P_PICK_AXE, IRON, HI_METAL),
WEPTOOL("�Ђ������_", "�S�̃t�b�N",
        0, 0, 0,  5,  30,  50,  2,  6, WHACK,  P_FLAIL,    IRON, HI_METAL),
WEPTOOL("���j�R�[���̊p", None,
        1, 1, 1,  0,  20, 100, 12, 12, PIERCE, P_UNICORN_HORN,
                                                           BONE, CLR_WHITE),
        /* 3.4.1: unicorn horn left classified as "magic" */
/* two unique tools;
 * not artifacts, despite the comment which used to be here
 */
OBJECT(OBJ("�F��̐C��", "�C��"),
       BITS(0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 0, P_NONE, GOLD),
       0, TOOL_CLASS, 0, 0, 10, 5000, 0, 0, 0, 0, 200, HI_GOLD),
OBJECT(OBJ("�J���̃x��", "��̃x��"),
       BITS(0, 0, 1, 0, 1, 1, 1, 1, 0, 0, 0, P_NONE, SILVER),
       0, TOOL_CLASS, 0, 0, 10, 5000, 0, 0, 0, 0, 50, HI_SILVER),
#undef TOOL
#undef WEPTOOL

/* Comestibles ... */
#define FOOD(name, prob, delay, wt, unk, tin, nutrition, color)         \
    OBJECT(OBJ(name, None),                                       \
           BITS(1, 1, unk, 0, 0, 0, 0, 0, 0, 0, 0, P_NONE, tin), 0,     \
           FOOD_CLASS, prob, delay, wt, nutrition / 20 + 5, 0, 0, 0, 0, \
           nutrition, color)
/* All types of food (except tins & corpses) must have a delay of at least 1.
 * Delay on corpses is computed and is weight dependant.
 * Domestic pets prefer tripe rations above all others.
 * Fortune cookies can be read, using them up without ingesting them.
 * Carrots improve your vision.
 * +0 tins contain monster meat.
 * +1 tins (of spinach) make you stronger (like Popeye).
 * Meatballs/sticks/rings are only created from objects via stone to flesh.
 */
/* meat */
FOOD("������",           140,  2, 10, 0, FLESH, 200, CLR_BROWN),
FOOD("����",               0,  1,  0, 0, FLESH,   0, CLR_BROWN),
FOOD("��",                85,  1,  1, 1, FLESH,  80, CLR_WHITE),
FOOD("�~�[�g�{�[��",       0,  1,  1, 0, FLESH,   5, CLR_BROWN),
FOOD("�~�[�g�X�e�B�b�N",   0,  1,  1, 0, FLESH,   5, CLR_BROWN),
FOOD("�傫�ȓ��̂����܂�", 0, 20,400, 0, FLESH,2000, CLR_BROWN),
/* special case because it's not mergable */
OBJECT(OBJ("meat ring", None),
       BITS(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FLESH),
       0, FOOD_CLASS, 0, 1, 5, 1, 0, 0, 0, 0, 5, CLR_BROWN),
/* pudding 'corpses' will turn into these and combine;
   must be in same order as the pudding monsters */
FOOD("�D�F�E�[�Y�̉�",     0,  2, 20, 0, FLESH,  20, CLR_GRAY),
FOOD("���F�v�����̉�",     0,  2, 20, 0, FLESH,  20, CLR_BROWN),
FOOD("�΃X���C���̉�",     0,  2, 20, 0, FLESH,  20, CLR_GREEN),
FOOD("���v�����̉�",       0,  2, 20, 0, FLESH,  20, CLR_BLACK),

/* fruits & veggies */
FOOD("���J��",             0,  1,  1, 0, VEGGY,  30, CLR_GREEN),
FOOD("���[�J���̗t",       3,  1,  1, 0, VEGGY,  30, CLR_GREEN),
FOOD("���",            15,  1,  2, 0, VEGGY,  50, CLR_RED),
FOOD("�I�����W",          10,  1,  2, 0, VEGGY,  80, CLR_ORANGE),
FOOD("�m�i�V",            10,  1,  2, 0, VEGGY,  50, CLR_BRIGHT_GREEN),
FOOD("������",            10,  1,  5, 0, VEGGY, 100, CLR_BRIGHT_GREEN),
FOOD("�o�i�i",            10,  1,  2, 0, VEGGY,  80, CLR_YELLOW),
FOOD("�ɂ񂶂�",          15,  1,  2, 0, VEGGY,  50, CLR_ORANGE),
FOOD("�g���J�u�g",         7,  1,  1, 0, VEGGY,  40, CLR_GREEN),
FOOD("�ɂ�ɂ�",           7,  1,  1, 0, VEGGY,  40, CLR_WHITE),
/* name of slime mold is changed based on player's OPTION=fruit:something
   and bones data might have differently named ones from prior games */
FOOD("�˂΂˂΃J�r",      75,  1,  5, 0, VEGGY, 250, HI_ORGANIC),

/* people food */
FOOD("���C�����[���[",     0,  1,  2, 0, VEGGY, 200, CLR_YELLOW),
FOOD("�N���[���p�C",      25,  1, 10, 0, VEGGY, 100, CLR_WHITE),
FOOD("�L�����f�B",        13,  1,  2, 0, VEGGY, 100, CLR_BROWN),
FOOD("�肢�N�b�L�[",      55,  1,  1, 0, VEGGY,  40, CLR_YELLOW),
FOOD("�p���P�[�L",        25,  2,  2, 0, VEGGY, 200, CLR_YELLOW),
FOOD("�����o�X",          20,  2,  5, 0, VEGGY, 800, CLR_WHITE),
FOOD("�N����",            20,  3, 15, 0, VEGGY, 600, HI_ORGANIC),
FOOD("�H��",             380,  5, 20, 0, VEGGY, 800, HI_ORGANIC),
FOOD("�j���[�V����",       0,  1, 10, 0, VEGGY, 400, HI_ORGANIC),
FOOD("�b���[�V����",       0,  1, 10, 0, VEGGY, 300, HI_ORGANIC),
/* tins have type specified by obj->spe (+1 for spinach, other implies
   flesh; negative specifies preparation method {homemade,boiled,&c})
   and by obj->corpsenm (type of monster flesh) */
FOOD("��",                  75,  0, 10, 1, METAL,   0, HI_METAL),
#undef FOOD

/* potions ... */
#define POTION(name,desc,mgc,power,prob,cost,color) \
    OBJECT(OBJ(name, desc),                                             \
           BITS(0, 1, 0, 0, mgc, 0, 0, 0, 0, 0, 0, P_NONE, GLASS),      \
           power, POTION_CLASS, prob, 0, 20, cost, 0, 0, 0, 0, 10, color)
POTION("�\�͊l���̖�",     "���r�[�F�̖�",  1, 0, 42, 300, CLR_RED),
POTION("�\�͉񕜂̖�",     "�s���N�F�̖�",  1, 0, 40, 100, CLR_BRIGHT_MAGENTA),
POTION("�����̖�",       "�I�����W�F�̖�",  1, CONFUSION, 42, 100, CLR_ORANGE),
POTION("�Ӗڂ̖�",             "���F�̖�",  1, BLINDED, 40, 150, CLR_YELLOW),
POTION("��Ⴢ̖�",     "�G�������h�F�̖�",  1, 0, 42, 300, CLR_BRIGHT_GREEN),
POTION("�����̖�",           "�×ΐF�̖�",  1, FAST, 42, 200, CLR_GREEN),
POTION("���V�̖�",         "�V�A���F�̖�",  1, LEVITATION, 42, 200, CLR_CYAN),
POTION("���o�̖�",             "���F�̖�",  1, HALLUC, 40, 100, CLR_CYAN),
POTION("�����̖�",       "���邢�F�̖�",  1, INVIS, 40, 150, CLR_BRIGHT_BLUE),
POTION("���̖�",       "�}�[���_�F�̖�",  1, SEE_INVIS, 42, 50, CLR_MAGENTA),
POTION("�񕜂̖�",           "�Ԏ��F�̖�",  1, 0, 57, 100, CLR_MAGENTA),
POTION("���񕜂̖�",         "�Ê��F�̖�",  1, 0, 47, 100, CLR_RED),
POTION("���x���A�b�v�̖�", "�~���N�F�̖�",  1, 0, 20, 300, CLR_WHITE),
POTION("�[�ւ̖�",     "�Q�������Ă����",  1, 0, 20, 200, CLR_BROWN),
POTION("������T����",   "�A�����Ă����",  1, 0, 40, 150, CLR_WHITE),
POTION("���̂�T����",   "�����łĂ����",  1, 0, 42, 150, CLR_GRAY),
POTION("���̖͂�",         "�܂��Ă����",  1, 0, 42, 150, CLR_WHITE),
POTION("�����̖�",       "�������Ă����",  1, 0, 42, 100, CLR_GRAY),
POTION("���S�񕜂̖�",           "������",  1, 0, 10, 200, CLR_BLACK),
POTION("�ω��̖�",             "���F�̖�",  1, 0, 10, 200, CLR_YELLOW),
POTION("�����ς炢�̖�",       "���F�̖�",  0, 0, 42,  50, CLR_BROWN),
POTION("�a�C�̖�",       "���A���Ă����",  0, 0, 42,  50, CLR_CYAN),
POTION("�t���[�c�W���[�X", "�A�C�ȐF�̖�",  0, 0, 42,  50, CLR_BLACK),
POTION("�_�̖�",                 "������",  0, 0, 10, 250, CLR_WHITE),
POTION("��",                   "�Z���̖�",  0, 0, 30, 250, CLR_BROWN),
/* fixed description
 */
POTION("��",                   "���F�̖�",  0, 0, 92, 100, CLR_CYAN),
#undef POTION

/* scrolls ... */
#define SCROLL(name,text,mgc,prob,cost) \
    OBJECT(OBJ(name, text),                                           \
           BITS(0, 1, 0, 0, mgc, 0, 0, 0, 0, 0, 0, P_NONE, PAPER),    \
           0, SCROLL_CLASS, prob, 0, 5, cost, 0, 0, 0, 0, 6, HI_PAPER)
SCROLL("�Z�ɖ��@�������銪��",     "�w�ւ�ւ�����x�Ə����ꂽ����",  1,  63,  80),
SCROLL("�Z��j�󂷂銪��",       "�w�ρ[�邠�ς����x�Ə����ꂽ����",  1,  45, 100),
SCROLL("���������������銪��", "�w���Ԃ񂷂��傴�[�x�Ə����ꂽ����",  1,  53, 100),
SCROLL("���������������銪��",     "�w�с[�����Ł[�x�Ə����ꂽ����",  1,  35, 100),
SCROLL("����̊���",                 "�w���납�ԁ[�x�Ə����ꂽ����",  1,  65,  80),
SCROLL("����ɖ��@�������銪��", "�w����ق炳�����x�Ə����ꂽ����",  1,  80,  60),
SCROLL("��������銪��",             "�w��Ȃ������x�Ə����ꂽ����",  1,  45, 200),
SCROLL("�����������Ȃ炷����",       "�w���낰�[��x�Ə����ꂽ����",  1,  15, 200),
SCROLL("�s�E�̊���",         "�w�΂����傤���񂾂��x�Ə����ꂽ����",  1,  15, 300),
SCROLL("���̊���",               "�w���������߂��Ɓx�Ə����ꂽ����",  1,  90,  50),
SCROLL("�u�Ԉړ��̊���",       "�w���[�������[�с[�x�Ə����ꂽ����",  1,  55, 100),
SCROLL("���݂�T������",           "�w���˂��˂��ˁx�Ə����ꂽ����",  1,  33, 100),
SCROLL("�H����T������",         "�w���Ԃ����������x�Ə����ꂽ����",  1,  25, 100),
SCROLL("���ʂ̊���",                   "�w�������[�x�Ə����ꂽ����",  1, 180,  20),
SCROLL("�n�}�̊���",                 "�w���Ⴂ���Ɓx�Ə����ꂽ����",  1,  45, 100),
SCROLL("�L���r���̊���",             "�w�܂����݂�x�Ə����ꂽ����",  1,  35, 200),
SCROLL("���̊���",             "�w�ǂ�����т񂫁[�x�Ə����ꂽ����",  1,  30, 100),
SCROLL("��n�̊���",         "�w�悵�̂€��ǁ[��x�Ə����ꂽ����",  1,  18, 200),
SCROLL("���̊���",                 "�w���ā[���݂�x�Ə����ꂽ����",  1,  15, 300),
SCROLL("�[�U�̊���",               "�w���Ⴂ���傭�x�Ə����ꂽ����",  1,  15, 300),
SCROLL("���L�_�̊���",           "�w�����҂��`��x�Ə����ꂽ����",  1,  15, 300),
    /* Extra descriptions, shuffled into use at start of new game.
     * Code in win/share/tilemap.c depends on SCR_STINKING_CLOUD preceding
     * these and on how many of them there are.  If a real scroll gets added
     * after stinking cloud or the number of extra descriptions changes,
     * tilemap.c must be modified to match.
     */
SCROLL(None,       "�w�͂ɂ�`��x�Ə����ꂽ����",  1,   0, 100),
SCROLL(None,       "�w�ۂ����Ƃȁx�Ə����ꂽ����",  1,   0, 100),
SCROLL(None, "�w���тт񂵂тт�x�Ə����ꂽ����",  1,   0, 100),
SCROLL(None,     "�w��������݂�x�Ə����ꂽ����",  1,   0, 100),
SCROLL(None,         "�wETAOIN SHRDLU�x�Ə����ꂽ����",  1,   0, 100),
SCROLL(None,           "�wLOREM IPSUM�x�Ə����ꂽ����",  1,   0, 100),
SCROLL(None,                 "�wFNORD�x�Ə����ꂽ����",  1,   0, 100), /* Illuminati */
SCROLL(None,               "�wKO BATE�x�Ə����ꂽ����",  1,   0, 100), /* Kurd Lasswitz */
SCROLL(None,         "�wABRA KA DABRA�x�Ə����ꂽ����",  1,   0, 100), /* traditional incantation */
SCROLL(None,          "�wASHPD SODALG�x�Ə����ꂽ����",  1,   0, 100), /* Portal */
SCROLL(None,               "�wZLORFIK�x�Ə����ꂽ����",  1,   0, 100), /* Zak McKracken */
SCROLL(None,         "�wGNIK SISI VLE�x�Ə����ꂽ����",  1,   0, 100), /* Zak McKracken */
SCROLL(None,       "�wHAPAX LEGOMENON�x�Ə����ꂽ����",  1,   0, 100),
SCROLL(None,     "�wEIRIS SAZUN IDISI�x�Ə����ꂽ����",  1,   0, 100), /* Merseburg Incantations */
SCROLL(None,       "�wPHOL ENDE WODAN�x�Ə����ꂽ����",  1,   0, 100), /* Merseburg Incantations */
SCROLL(None,                 "�wGHOTI�x�Ə����ꂽ����",  1,   0, 100), /* pronounced as 'fish',
                                                        George Bernard Shaw */
SCROLL(None, "�wMAPIRO MAHAMA DIROMAT�x�Ə����ꂽ����", 1, 0, 100), /* Wizardry */
SCROLL(None,     "�wVAS CORP BET MANI�x�Ə����ꂽ����",  1,   0, 100), /* Ultima */
SCROLL(None,               "�wXOR OTA�x�Ə����ꂽ����",  1,   0, 100), /* Aarne Haapakoski */
SCROLL(None,    "�wSTRC PRST SKRZ KRK�x�Ə����ꂽ����",  1,   0, 100), /* Czech and Slovak
                                                        tongue-twister */
    /* These must come last because they have special fixed descriptions.
     */
#ifdef MAIL
SCROLL("�莆�̊���",         "����̉����ꂽ����",  0,   0,   0),
#endif
SCROLL("�����̊���",           "���x���̂Ȃ�����",  0,  28,  60),
#undef SCROLL

/* spellbooks ... */
/* expanding beyond 52 spells would require changes in spellcasting
   or imposition of a limit on number of spells hero can know because
   they are currently assigned successive letters, a-zA-Z, when learned */
#define SPELL(name,desc,sub,prob,delay,level,mgc,dir,color)  \
    OBJECT(OBJ(name, desc),                                             \
           BITS(0, 0, 0, 0, mgc, 0, 0, 0, 0, 0, dir, sub, PAPER),       \
           0, SPBOOK_CLASS, prob, delay, 50, level * 100,               \
           0, 0, 0, level, 20, color)
SPELL("���@��̖��@��",   "�r�玆�̖��@��",
      P_MATTER_SPELL,      20,  6, 5, 1, RAY, HI_PAPER),
SPELL("��̖��@��",       "�q����̖��@��",
      P_ATTACK_SPELL,      45,  2, 2, 1, RAY, HI_PAPER),
SPELL("�΂̋ʂ̖��@��",   "�ڂ�ڂ�̖��@��",
      P_ATTACK_SPELL,      20,  4, 4, 1, RAY, HI_PAPER),
SPELL("��C�̖��@��",     "�y�[�W�̐܂�ꂽ���@��",
      P_ATTACK_SPELL,      10,  7, 4, 1, RAY, HI_PAPER),
SPELL("����̖��@��",     "�܂���̖��@��",
      P_ENCHANTMENT_SPELL, 50,  1, 1, 1, RAY, HI_PAPER),
SPELL("���̎w�̖��@��",   "�悲�ꂽ���@��",
      P_ATTACK_SPELL,       5, 10, 7, 1, RAY, HI_PAPER),
SPELL("����̖��@��",     "�z�n�̖��@��",
      P_DIVINATION_SPELL,  45,  1, 1, 1, NODIR, HI_CLOTH),
SPELL("������T�����@��", "leathery",
      P_DIVINATION_SPELL,  43,  1, 1, 1, NODIR, HI_LEATHER),
SPELL("�񕜂̖��@��",     "�������@��",
      P_HEALING_SPELL,     40,  2, 1, 1, IMMEDIATE, CLR_WHITE),
SPELL("�J���̖��@��",     "�s���N�F�̖��@��",
      P_MATTER_SPELL,      35,  1, 1, 1, IMMEDIATE, CLR_BRIGHT_MAGENTA),
SPELL("�Ռ��̖��@��",     "�Ԃ����@��",
      P_ATTACK_SPELL,      35,  2, 1, 1, IMMEDIATE, CLR_RED),
SPELL("�����̖��@��",     "�I�����W�F�̖��@��",
      P_ENCHANTMENT_SPELL, 30,  2, 2, 1, IMMEDIATE, CLR_ORANGE),
SPELL("�Ӗڂ�������@��", "���F�����@��",
      P_HEALING_SPELL,     25,  2, 2, 1, IMMEDIATE, CLR_YELLOW),
SPELL("�E�̖͂��@��",     "�r���[�h�̖��@��",
      P_ATTACK_SPELL,      10,  2, 2, 1, IMMEDIATE, CLR_MAGENTA),
SPELL("�����̖��@��",     "�W�ΐF�̖��@��",
      P_ENCHANTMENT_SPELL, 30,  2, 2, 1, IMMEDIATE, CLR_BRIGHT_GREEN),
SPELL("�{���̖��@��",     "�Z�ΐF�̖��@��",
      P_MATTER_SPELL,      30,  3, 2, 1, IMMEDIATE, CLR_GREEN),
SPELL("�����𑢂閂�@��", "�ΐF�̖��@��",
      P_CLERIC_SPELL,      35,  3, 2, 1, NODIR, CLR_BRIGHT_CYAN),
SPELL("�H����T�����@��", "�V�A���F�̖��@��",
      P_DIVINATION_SPELL,  30,  3, 2, 1, NODIR, CLR_CYAN),
SPELL("���|�̖��@��",     "�W�̖��@��",
      P_ENCHANTMENT_SPELL, 25,  3, 3, 1, NODIR, CLR_BRIGHT_BLUE),
SPELL("�痢��̖��@��",   "�Z�̖��@��",
      P_DIVINATION_SPELL,  15,  3, 3, 1, NODIR, CLR_BLUE),
SPELL("�a�C��������@��", "���F�̖��@��",
      P_HEALING_SPELL,     32,  3, 3, 1, NODIR, CLR_BLUE),
SPELL("�����̖��@��",     "�}�[���_�F�̖��@��",
      P_ENCHANTMENT_SPELL, 20,  3, 3, 1, IMMEDIATE, CLR_MAGENTA),
SPELL("���U�̖��@��",     "���F�̖��@��",
      P_ESCAPE_SPELL,      33,  4, 3, 1, NODIR, CLR_MAGENTA),
SPELL("�슴�̖��@��",     "�X�~���F�̖��@��",
      P_DIVINATION_SPELL,  20,  4, 3, 1, NODIR, CLR_MAGENTA),
SPELL("���V�̖��@��",     "�����F�̖��@��",
      P_ESCAPE_SPELL,      20,  4, 4, 1, NODIR, CLR_BROWN),
SPELL("���񕜂̖��@��",   "���V���̖��@��",
      P_HEALING_SPELL,     27,  5, 3, 1, IMMEDIATE, CLR_GREEN),
SPELL("�\�͉񕜂̖��@��", "�W���F�̖��@��",
      P_HEALING_SPELL,     25,  5, 4, 1, NODIR, CLR_BROWN),
SPELL("�����̖��@��",     "�Z���F�̖��@��",
      P_ESCAPE_SPELL,      25,  5, 4, 1, NODIR, CLR_BROWN),
SPELL("���T�����@��",   "�D�F�̖��@��",
      P_DIVINATION_SPELL,  20,  5, 4, 1, NODIR, CLR_GRAY),
SPELL("����̖��@��",     "�����Ⴍ����̖��@��",
      P_CLERIC_SPELL,      25,  5, 3, 1, NODIR, HI_PAPER),
SPELL("�n�}�̖��@��",     "�ق�����ۂ����@��",
      P_DIVINATION_SPELL,  18,  7, 5, 1, NODIR, HI_PAPER),
SPELL("���ʂ̖��@��",     "���̖��@��",
      P_DIVINATION_SPELL,  20,  6, 3, 1, NODIR, HI_COPPER),
SPELL("�h���̖��@��",     "���̖��@��",
      P_CLERIC_SPELL,      16,  8, 6, 1, IMMEDIATE, HI_COPPER),
SPELL("�ω��̖��@��",     "��̖��@��",
      P_MATTER_SPELL,      10,  8, 6, 1, IMMEDIATE, HI_SILVER),
SPELL("�u�Ԉړ��̖��@��", "���̖��@��",
      P_ESCAPE_SPELL,      15,  6, 6, 1, IMMEDIATE, HI_GOLD),
SPELL("�����̖��@��",     "����т₩�Ȗ��@��",
      P_CLERIC_SPELL,      10,  7, 6, 1, NODIR, CLR_WHITE),
SPELL("���͉��̖��@��",   "�P�����@��",
      P_MATTER_SPELL,      15,  8, 7, 1, IMMEDIATE, CLR_WHITE),
SPELL("���̖��@��",     "���F�̖��@��",
      P_CLERIC_SPELL,      18,  3, 1, 1, NODIR, HI_PAPER),
SPELL("����̖��@��",     "�����F�̖��@��",
      P_ESCAPE_SPELL,      20,  3, 1, 1, IMMEDIATE, HI_PAPER),
SPELL("��̖��@��",     "�Z���F�̖��@��",
      P_HEALING_SPELL,     15,  1, 3, 1, IMMEDIATE, HI_PAPER),
#if 0 /* DEFERRED */
/* from slash'em, create a tame critter which explodes when attacking,
   damaging adjacent creatures--friend or foe--and dying in the process */
SPELL("flame sphere",     "���̖��@��",
      P_MATTER_SPELL,      20,  2, 1, 1, NODIR, CLR_BROWN),
SPELL("freeze sphere",    "�ł��\���̖��@��",
      P_MATTER_SPELL,      20,  2, 1, 1, NODIR, CLR_BROWN),
#endif
/* books with fixed descriptions
 */
SPELL("�����̖��@��", "�^�����Ȗ��@��", P_NONE, 18, 0, 0, 0, 0, HI_PAPER),
/* tribute book for 3.6 */
OBJECT(OBJ("����", "�y�[�p�[�o�b�N�̖��@��"),
       BITS(0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, P_NONE, PAPER),
       0, SPBOOK_CLASS, 0, 0, 0, 20, 0, 0, 0, 1, 20, CLR_BRIGHT_BLUE),
/* a special, one of a kind, spellbook */
OBJECT(OBJ("���҂̏�", "�p�s���X�̖��@��"),
       BITS(0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 0, P_NONE, PAPER),
       0, SPBOOK_CLASS, 0, 0, 20, 10000, 0, 0, 0, 7, 20, HI_PAPER),
#undef SPELL

/* wands ... */
#define WAND(name,typ,prob,cost,mgc,dir,metal,color) \
    OBJECT(OBJ(name, typ),                                              \
           BITS(0, 0, 1, 0, mgc, 1, 0, 0, 0, 0, dir, P_NONE, metal),    \
           0, WAND_CLASS, prob, 0, 7, cost, 0, 0, 0, 0, 30, color)
WAND("����̏�",             "�K���X�̏�", 95, 100, 1, NODIR, GLASS, HI_GLASS),
WAND("���T���̏�",
                        "�o���T�̏�", 50, 150, 1, NODIR, WOOD, HI_WOOD),
WAND("�[�ւ̏�",               "�����̏�", 15, 150, 1, NODIR, GLASS, HI_GLASS),
WAND("�����𑢂��",             "���̏�", 45, 200, 1, NODIR, WOOD, HI_WOOD),
WAND("�肢�̏�",                 "���̏�",  5, 500, 1, NODIR, WOOD, HI_WOOD),
WAND("�P�Ȃ��",                 "�~�̏�", 25, 100, 0, IMMEDIATE, WOOD, HI_WOOD),
WAND("�Ռ��̏�",               "���h�̏�", 75, 150, 1, IMMEDIATE, WOOD, HI_WOOD),
WAND("�������̏�",           "�嗝�΂̏�", 45, 150, 1, IMMEDIATE, MINERAL, HI_MINERAL),
WAND("�����̏�",             "�u���L�̏�", 50, 150, 1, IMMEDIATE, METAL, HI_METAL),
WAND("�����̏�",               "�^�J�̏�", 50, 150, 1, IMMEDIATE, COPPER, HI_COPPER),
WAND("�h���̏�",                 "���̏�", 50, 150, 1, IMMEDIATE, COPPER, HI_COPPER),
WAND("�ω��̏�",                 "��̏�", 45, 200, 1, IMMEDIATE, SILVER, HI_SILVER),
WAND("���͉��̏�",         "�v���`�i�̏�", 45, 200, 1, IMMEDIATE, PLATINUM, CLR_WHITE),
WAND("�u�Ԉړ��̏�",     "�C���W�E���̏�", 45, 200, 1, IMMEDIATE, METAL,
                                                             CLR_BRIGHT_CYAN),
WAND("�J���̏�",               "�����̏�", 25, 150, 1, IMMEDIATE, METAL, HI_METAL),
WAND("�{���̏�",       "�A���~�j�E���̏�", 25, 150, 1, IMMEDIATE, METAL, HI_METAL),
WAND("�T�������",       "�E���j�E���̏�", 30, 150, 1, IMMEDIATE, METAL, HI_METAL),
WAND("���@��̏�",               "�S�̏�", 55, 150, 1, RAY, IRON, HI_METAL),
WAND("��̏�",                 "�|�S�̏�", 50, 150, 1, RAY, IRON, HI_METAL),
WAND("���̏�",               "�Z�p�`�̏�", 40, 175, 1, RAY, IRON, HI_METAL),
WAND("����̏�",                 "�Z����", 40, 175, 1, RAY, IRON, HI_METAL),
WAND("����̏�", "���[�������̏����ꂽ��", 50, 175, 1, RAY, IRON, HI_METAL),
WAND("���̏�",                   "������",  5, 500, 1, RAY, IRON, HI_METAL),
WAND("���̏�",                 "�Ȃ�����", 40, 175, 1, RAY, IRON, HI_METAL),
/* extra descriptions, shuffled into use at start of new game */
WAND(None,                     "�񍳂̏�",  0, 150, 1, 0, WOOD, HI_WOOD),
WAND(None,     "�����т̑ł�����ꂽ��",  0, 150, 1, 0, IRON, HI_METAL),
WAND(None,           "��΂����߂�ꂽ��",  0, 150, 1, 0, IRON, HI_MINERAL),
#undef WAND

/* coins ... - so far, gold is all there is */
#define COIN(name,prob,metal,worth) \
    OBJECT(OBJ(name, None),                                        \
           BITS(0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, P_NONE, metal),    \
           0, COIN_CLASS, prob, 0, 1, worth, 0, 0, 0, 0, 0, HI_GOLD)
COIN("����", 1000, GOLD, 1),
#undef COIN

/* gems ... - includes stones and rocks but not boulders */
#define GEM(name,desc,prob,wt,gval,nutr,mohs,glass,color) \
    OBJECT(OBJ(name, desc),                                             \
           BITS(0, 1, 0, 0, 0, 0, 0, 0, 0,                              \
                HARDGEM(mohs), 0, -P_SLING, glass),                     \
           0, GEM_CLASS, prob, 0, 1, gval, 3, 3, 0, 0, nutr, color)
#define ROCK(name,desc,kn,prob,wt,gval,sdam,ldam,mgc,nutr,mohs,glass,color) \
    OBJECT(OBJ(name, desc),                                             \
           BITS(kn, 1, 0, 0, mgc, 0, 0, 0, 0,                           \
                HARDGEM(mohs), 0, -P_SLING, glass),                     \
           0, GEM_CLASS, prob, 0, wt, gval, sdam, ldam, 0, 0, nutr, color)
GEM("�f�B���W�E���̌���", "������",  2, 1, 4500, 15,  5, GEMSTONE, CLR_WHITE),
GEM("�_�C�������h",       "������",  3, 1, 4000, 15, 10, GEMSTONE, CLR_WHITE),
GEM("���r�[",             "�Ԃ���",  4, 1, 3500, 15,  9, GEMSTONE, CLR_RED),
GEM("�򐅏�",           "��F�̐�",  3, 1, 3250, 15,  9, GEMSTONE, CLR_ORANGE),
GEM("�T�t�@�C�A",         "����",  4, 1, 3000, 15,  9, GEMSTONE, CLR_BLUE),
GEM("���I�p�[��",         "������",  3, 1, 2500, 15,  8, GEMSTONE, CLR_BLACK),
GEM("�G�������h",         "�΂̐�",  5, 1, 2500, 15,  8, GEMSTONE, CLR_GREEN),
GEM("�g���R��",           "�΂̐�",  6, 1, 2000, 15,  6, GEMSTONE, CLR_GREEN),
GEM("������",           "���F����",  4, 1, 1500, 15,  6, GEMSTONE, CLR_YELLOW),
GEM("�A�N�A�}����",       "�΂̐�",  6, 1, 1500, 15,  8, GEMSTONE, CLR_GREEN),
GEM("����",           "�����F�̐�",  8, 1, 1000, 15,  2, GEMSTONE, CLR_BROWN),
GEM("�g�p�[�Y",       "�����F�̐�", 10, 1,  900, 15,  8, GEMSTONE, CLR_BROWN),
GEM("����",               "������",  6, 1,  850, 15,  7, GEMSTONE, CLR_BLACK),
GEM("�I�p�[��",           "������", 12, 1,  800, 15,  6, GEMSTONE, CLR_WHITE),
GEM("���ΐ�",           "���F����",  8, 1,  700, 15,  5, GEMSTONE, CLR_YELLOW),
GEM("�K�[�l�b�g",         "�Ԃ���", 12, 1,  700, 15,  7, GEMSTONE, CLR_RED),
GEM("�A���W�X�g",         "���̐�", 14, 1,  600, 15,  7, GEMSTONE, CLR_MAGENTA),
GEM("�W���X�p�[",         "�Ԃ���", 15, 1,  500, 15,  7, GEMSTONE, CLR_RED),
GEM("�t���I���C�g",       "���̐�", 15, 1,  400, 15,  4, GEMSTONE, CLR_MAGENTA),
GEM("��ࠐ�",             "������",  9, 1,  200, 15,  6, GEMSTONE, CLR_BLACK),
GEM("�߂̂�",           "��F�̐�", 12, 1,  200, 15,  6, GEMSTONE, CLR_ORANGE),
GEM("�Ђ���",             "�΂̐�", 10, 1,  300, 15,  6, GEMSTONE, CLR_GREEN),
GEM("�����K���X", "������",
    77, 1, 0, 6, 5, GLASS, CLR_WHITE),
GEM("���K���X", "����",
    77, 1, 0, 6, 5, GLASS, CLR_BLUE),
GEM("�Ԃ��K���X", "�Ԃ���",
    77, 1, 0, 6, 5, GLASS, CLR_RED),
GEM("�����F�̃K���X", "�����F�̐�",
    77, 1, 0, 6, 5, GLASS, CLR_BROWN),
GEM("��F�̃K���X", "��F�̐�",
    76, 1, 0, 6, 5, GLASS, CLR_ORANGE),
GEM("���F�̃K���X", "���F����",
    77, 1, 0, 6, 5, GLASS, CLR_YELLOW),
GEM("���F�̃K���X", "������",
    76, 1, 0, 6, 5, GLASS, CLR_BLACK),
GEM("�΂̃K���X", "�΂̐�",
    77, 1, 0, 6, 5, GLASS, CLR_GREEN),
GEM("���̃K���X", "���̐�",
    77, 1, 0, 6, 5, GLASS, CLR_MAGENTA),

/* Placement note: there is a wishable subrange for
 * "gray stones" in the o_ranges[] array in objnam.c
 * that is currently everything between luckstones and flint
 * (inclusive).
 */
ROCK("�K���̐�", "�D�F�̕��", 0,  10,  10, 60, 3, 3, 1, 10, 7, MINERAL, CLR_GRAY),
ROCK("�d��", "�D�F�̕��",     0,  10, 500,  1, 3, 3, 1, 10, 6, MINERAL, CLR_GRAY),
ROCK("������", "�D�F�̕��",   0,   8,  10, 45, 3, 3, 1, 10, 6, MINERAL, CLR_GRAY),
ROCK("�Αł���", "�D�F�̕��", 0,  10,  10,  1, 6, 6, 0, 10, 7, MINERAL, CLR_GRAY),
ROCK("��", None,               1, 100,  10,  0, 3, 3, 0, 10, 7, MINERAL, CLR_GRAY),
#undef GEM
#undef ROCK

/* miscellaneous ... */
/* Note: boulders and rocks are not normally created at random; the
 * probabilities only come into effect when you try to polymorph them.
 * Boulders weigh more than MAX_CARR_CAP; statues use corpsenm to take
 * on a specific type and may act as containers (both affect weight).
 */
OBJECT(OBJ("��", None),
       BITS(1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, P_NONE, MINERAL), 0,
       ROCK_CLASS, 100, 0, 6000, 0, 20, 20, 0, 0, 2000, HI_MINERAL),
OBJECT(OBJ("��", None),
       BITS(1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, P_NONE, MINERAL), 0,
       ROCK_CLASS, 900, 0, 2500, 0, 20, 20, 0, 0, 2500, CLR_WHITE),

OBJECT(OBJ("�d���S��", None),
       BITS(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, WHACK, P_NONE, IRON), 0,
       BALL_CLASS, 1000, 0, 480, 10, 25, 25, 0, 0, 200, HI_METAL),
        /* +d4 when "very heavy" */
OBJECT(OBJ("�S�̍�", None),
       BITS(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, WHACK, P_NONE, IRON), 0,
       CHAIN_CLASS, 1000, 0, 120, 0, 4, 4, 0, 0, 200, HI_METAL),
        /* +1 both l & s */

/* Venom is normally a transitory missile (spit by various creatures)
 * but can be wished for in wizard mode so could occur in bones data.
 */
OBJECT(OBJ("�Ӗڂ̓ŉt", "�ŉt�̂��Ԃ�"),
       BITS(0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, P_NONE, LIQUID), 0,
       VENOM_CLASS, 500, 0, 1, 0, 0, 0, 0, 0, 0, HI_ORGANIC),
OBJECT(OBJ("�_�̓ŉt", "�ŉt�̂��Ԃ�"),
       BITS(0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, P_NONE, LIQUID), 0,
       VENOM_CLASS, 500, 0, 1, 0, 6, 6, 0, 0, 0, HI_ORGANIC),
        /* +d6 small or large */

/* fencepost, the deadly Array Terminator -- name [1st arg] *must* be NULL */
OBJECT(OBJ(None, None),
       BITS(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, P_NONE, 0), 0,
       ILLOBJ_CLASS, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
}; /* objects[] */

#ifndef OBJECTS_PASS_2_

/* perform recursive compilation for second structure */
#undef OBJ
#undef OBJECT
#define OBJECTS_PASS_2_
#include "objects.c"

/* clang-format on */
/* *INDENT-ON* */

void NDECL(objects_init);

/* dummy routine used to force linkage */
void
objects_init()
{
    return;
}

#endif /* !OBJECTS_PASS_2_ */

/*objects.c*/
