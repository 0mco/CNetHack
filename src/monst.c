/* NetHack 3.6	monst.c	$NHDT-Date: 1510531569 2017/11/13 00:06:09 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.59 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Michael Allison, 2006. */
/* NetHack may be freely redistributed.  See license for details. */

#include "config.h"
#include "permonst.h"
#include "monsym.h"

#define NO_ATTK    \
    {              \
        0, 0, 0, 0 \
    }

#define WT_ELF 800
#define WT_DRAGON 4500

#ifdef C
#undef C
#endif
#ifdef TEXTCOLOR
#include "color.h"
#define C(color) color
#define HI_DOMESTIC CLR_WHITE /* use for player + friendlies */
#define HI_LORD CLR_MAGENTA
#else
#define C(color)
#endif

void NDECL(monst_init);
/*
 *      Entry Format:   (from permonst.h)
 *
 *      name, symbol (S_* defines),
 *      difficulty level, move rate, armor class, magic resistance,
 *      alignment, creation/geno flags (G_* defines),
 *      6 * attack structs ( type , damage-type, # dice, # sides ),
 *      weight (WT_* defines), nutritional value, extension length,
 *      sounds made (MS_* defines), physical size (MZ_* defines),
 *      resistances, resistances conferred (both MR_* defines),
 *      3 * flag bitmaps (M1_*, M2_*, and M3_* defines respectively)
 *      symbol color (C(x) macro)
 *
 *      For AT_BREA attacks, '# sides' is ignored; 6 is used for most
 *      damage types, 25 for sleep, not applicable for death or poison.
 */
// >>> CN_TS
/* #define MON(nam, sym, lvl, gen, atk, siz, mr1, mr2, flg1, flg2, flg3, col) \ */
/*     {                                                                      \ */
/*         nam, sym, lvl, gen, atk, siz, mr1, mr2, flg1, flg2, flg3, C(col)   \ */
/*     } */
#define MON(nam, cnam, sym, lvl, gen, atk, siz, mr1, mr2, flg1, flg2, flg3, col) \
    {                                                                      \
        nam, cnam, sym, lvl, gen, atk, siz, mr1, mr2, flg1, flg2, flg3, C(col)   \
    }
// <<< CN_TS
/* LVL() and SIZ() collect several fields to cut down on # of args for MON()
 */
#define LVL(lvl, mov, ac, mr, aln) lvl, mov, ac, mr, aln
#define SIZ(wt, nut, snd, siz) wt, nut, snd, siz
/* ATTK() and A() are to avoid braces and commas within args to MON() */
#define ATTK(at, ad, n, d) \
    {                      \
        at, ad, n, d       \
    }
#define A(a1, a2, a3, a4, a5, a6) \
    {                             \
        a1, a2, a3, a4, a5, a6    \
    }

/*
 *      Rule #1:        monsters of a given class are contiguous in the
 *                      mons[] array.
 *
 *      Rule #2:        monsters of a given class are presented in ascending
 *                      order of strength.
 *
 *      Rule #3:        monster frequency is included in the geno mask;
 *                      the frequency can be from 0 to 7.  0's will also
 *                      be skipped during generation.
 *
 *      Rule #4:        monster subclasses (e.g. giants) should be kept
 *                      together, unless it violates Rule 2.  NOGEN monsters
 *                      won't violate Rule 2.
 *
 * Guidelines for color assignment:
 *
 *      * Use the same color for all `growth stages' of a monster (ex.
 *        little dog/big dog, baby naga/full-grown naga.
 *
 *      * Use colors given in names wherever possible. If the class has `real'
 *        members with strong color associations, use those.
 *
 *      * Favor `cool' colors for cold-resistant monsters, `warm' ones for
 *        fire-resistant ones.
 *
 *      * Try to reserve purple (magenta) for powerful `ruler' monsters (queen
 *        bee, kobold lord, &c.).
 *
 *      * Subject to all these constraints, try to use color to make as many
 *        distinctions as the / command (that is, within a monster letter
 *        distinct names should map to distinct colors).
 *
 * The aim in assigning colors is to be consistent enough so a player can
 * become `intuitive' about them, deducing some or all of these rules
 * unconsciously. Use your common sense.
 */

// >>> CN_TS
/* #ifndef SPLITMON_2 */
/* NEARDATA struct permonst mons[] = { */
/*     #<{(| */
/*      * ants */
/*      |)}># */
/*     MON("giant ant", S_ANT, LVL(2, 18, 3, 0, 0), (G_GENO | G_SGROUP | 3), */
/*         A(ATTK(AT_BITE, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(10, 10, MS_SILENT, MZ_TINY), 0, 0, */
/*         M1_ANIMAL | M1_NOHANDS | M1_OVIPAROUS | M1_CARNIVORE, M2_HOSTILE, 0, */
/*         CLR_BROWN), */
/*     MON("killer bee", S_ANT, LVL(1, 18, -1, 0, 0), (G_GENO | G_LGROUP | 2), */
/*         A(ATTK(AT_STNG, AD_DRST, 1, 3), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(1, 5, MS_BUZZ, MZ_TINY), MR_POISON, MR_POISON, */
/*         M1_ANIMAL | M1_FLY | M1_NOHANDS | M1_POIS, M2_HOSTILE | M2_FEMALE, 0, */
/*         CLR_YELLOW), */
/*     MON("soldier ant", S_ANT, LVL(3, 18, 3, 0, 0), (G_GENO | G_SGROUP | 2), */
/*         A(ATTK(AT_BITE, AD_PHYS, 2, 4), ATTK(AT_STNG, AD_DRST, 3, 4), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(20, 5, MS_SILENT, MZ_TINY), MR_POISON, MR_POISON, */
/*         M1_ANIMAL | M1_NOHANDS | M1_OVIPAROUS | M1_POIS | M1_CARNIVORE, */
/*         M2_HOSTILE, 0, CLR_BLUE), */
/*     MON("fire ant", S_ANT, LVL(3, 18, 3, 10, 0), (G_GENO | G_SGROUP | 1), */
/*         A(ATTK(AT_BITE, AD_PHYS, 2, 4), ATTK(AT_BITE, AD_FIRE, 2, 4), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(30, 10, MS_SILENT, MZ_TINY), MR_FIRE, MR_FIRE, */
/*         M1_ANIMAL | M1_NOHANDS | M1_OVIPAROUS | M1_CARNIVORE, M2_HOSTILE, */
/*         M3_INFRAVISIBLE, CLR_RED), */
/*     MON("giant beetle", S_ANT, LVL(5, 6, 4, 0, 0), (G_GENO | 3), */
/*         A(ATTK(AT_BITE, AD_PHYS, 3, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(10, 10, MS_SILENT, MZ_LARGE), MR_POISON, MR_POISON, */
/*         M1_ANIMAL | M1_NOHANDS | M1_POIS | M1_CARNIVORE, M2_HOSTILE, 0, */
/*         CLR_BLACK), */
/*     MON("queen bee", S_ANT, LVL(9, 24, -4, 0, 0), (G_GENO | G_NOGEN), */
/*         A(ATTK(AT_STNG, AD_DRST, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(1, 5, MS_BUZZ, MZ_TINY), MR_POISON, MR_POISON, */
/*         M1_ANIMAL | M1_FLY | M1_NOHANDS | M1_OVIPAROUS | M1_POIS, */
/*         M2_HOSTILE | M2_FEMALE | M2_PRINCE, 0, HI_LORD), */
/*     #<{(| */
/*      * blobs */
/*      |)}># */
/*     MON("acid blob", S_BLOB, LVL(1, 3, 8, 0, 0), (G_GENO | 2), */
/*         A(ATTK(AT_NONE, AD_ACID, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(30, 10, MS_SILENT, MZ_TINY), */
/*         MR_SLEEP | MR_POISON | MR_ACID | MR_STONE, MR_STONE, */
/*         M1_BREATHLESS | M1_AMORPHOUS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD */
/*             | M1_MINDLESS | M1_ACID, */
/*         M2_WANDER | M2_NEUTER, 0, CLR_GREEN), */
/*     MON("quivering blob", S_BLOB, LVL(5, 1, 8, 0, 0), (G_GENO | 2), */
/*         A(ATTK(AT_TUCH, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(200, 100, MS_SILENT, MZ_SMALL), MR_SLEEP | MR_POISON, MR_POISON, */
/*         M1_NOEYES | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS, */
/*         M2_WANDER | M2_HOSTILE | M2_NEUTER, 0, CLR_WHITE), */
/*     MON("gelatinous cube", S_BLOB, LVL(6, 6, 8, 0, 0), (G_GENO | 2), */
/*         A(ATTK(AT_TUCH, AD_PLYS, 2, 4), ATTK(AT_NONE, AD_PLYS, 1, 4), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(600, 150, MS_SILENT, MZ_LARGE), */
/*         MR_FIRE | MR_COLD | MR_ELEC | MR_SLEEP | MR_POISON | MR_ACID */
/*             | MR_STONE, */
/*         MR_FIRE | MR_COLD | MR_ELEC | MR_SLEEP, */
/*         M1_NOEYES | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS | M1_OMNIVORE */
/*             | M1_ACID, */
/*         M2_WANDER | M2_HOSTILE | M2_NEUTER, 0, CLR_CYAN), */
/*     #<{(| */
/*      * cockatrice */
/*      |)}># */
/*     MON("chickatrice", S_COCKATRICE, LVL(4, 4, 8, 30, 0), */
/*         (G_GENO | G_SGROUP | 1), */
/*         A(ATTK(AT_BITE, AD_PHYS, 1, 2), ATTK(AT_TUCH, AD_STON, 0, 0), */
/*           ATTK(AT_NONE, AD_STON, 0, 0), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(10, 10, MS_HISS, MZ_TINY), MR_POISON | MR_STONE, */
/*         MR_POISON | MR_STONE, M1_ANIMAL | M1_NOHANDS | M1_OMNIVORE, */
/*         M2_HOSTILE, M3_INFRAVISIBLE, CLR_BROWN), */
/*     MON("cockatrice", S_COCKATRICE, LVL(5, 6, 6, 30, 0), (G_GENO | 5), */
/*         A(ATTK(AT_BITE, AD_PHYS, 1, 3), ATTK(AT_TUCH, AD_STON, 0, 0), */
/*           ATTK(AT_NONE, AD_STON, 0, 0), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(30, 30, MS_HISS, MZ_SMALL), MR_POISON | MR_STONE, */
/*         MR_POISON | MR_STONE, */
/*         M1_ANIMAL | M1_NOHANDS | M1_OMNIVORE | M1_OVIPAROUS, M2_HOSTILE, */
/*         M3_INFRAVISIBLE, CLR_YELLOW), */
/*     MON("pyrolisk", S_COCKATRICE, LVL(6, 6, 6, 30, 0), (G_GENO | 1), */
/*         A(ATTK(AT_GAZE, AD_FIRE, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(30, 30, MS_HISS, MZ_SMALL), MR_POISON | MR_FIRE, */
/*         MR_POISON | MR_FIRE, */
/*         M1_ANIMAL | M1_NOHANDS | M1_OMNIVORE | M1_OVIPAROUS, M2_HOSTILE, */
/*         M3_INFRAVISIBLE, CLR_RED), */
/*     #<{(| */
/*      * dogs & other canines */
/*      |)}># */
/*     MON("jackal", S_DOG, LVL(0, 12, 7, 0, 0), (G_GENO | G_SGROUP | 3), */
/*         A(ATTK(AT_BITE, AD_PHYS, 1, 2), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(300, 250, MS_BARK, MZ_SMALL), 0, 0, */
/*         M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, M3_INFRAVISIBLE, */
/*         CLR_BROWN), */
/*     MON("fox", S_DOG, LVL(0, 15, 7, 0, 0), (G_GENO | 1), */
/*         A(ATTK(AT_BITE, AD_PHYS, 1, 3), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(300, 250, MS_BARK, MZ_SMALL), 0, 0, */
/*         M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, M3_INFRAVISIBLE, */
/*         CLR_RED), */
/*     MON("coyote", S_DOG, LVL(1, 12, 7, 0, 0), (G_GENO | G_SGROUP | 1), */
/*         A(ATTK(AT_BITE, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(300, 250, MS_BARK, MZ_SMALL), 0, 0, */
/*         M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, M3_INFRAVISIBLE, */
/*         CLR_BROWN), */
/*     MON("werejackal", S_DOG, LVL(2, 12, 7, 10, -7), (G_NOGEN | G_NOCORPSE), */
/*         A(ATTK(AT_BITE, AD_WERE, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(300, 250, MS_BARK, MZ_SMALL), MR_POISON, 0, */
/*         M1_NOHANDS | M1_POIS | M1_REGEN | M1_CARNIVORE, */
/*         M2_NOPOLY | M2_WERE | M2_HOSTILE, M3_INFRAVISIBLE, CLR_BROWN), */
/*     MON("little dog", S_DOG, LVL(2, 18, 6, 0, 0), (G_GENO | 1), */
/*         A(ATTK(AT_BITE, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(150, 150, MS_BARK, MZ_SMALL), 0, 0, */
/*         M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_DOMESTIC, M3_INFRAVISIBLE, */
/*         HI_DOMESTIC), */
/*     MON("dingo", S_DOG, LVL(4, 16, 5, 0, 0), (G_GENO | 1), */
/*         A(ATTK(AT_BITE, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(400, 200, MS_BARK, MZ_MEDIUM), 0, 0, */
/*         M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, M3_INFRAVISIBLE, */
/*         CLR_YELLOW), */
/*     MON("dog", S_DOG, LVL(4, 16, 5, 0, 0), (G_GENO | 1), */
/*         A(ATTK(AT_BITE, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(400, 200, MS_BARK, MZ_MEDIUM), 0, 0, */
/*         M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_DOMESTIC, M3_INFRAVISIBLE, */
/*         HI_DOMESTIC), */
/*     MON("large dog", S_DOG, LVL(6, 15, 4, 0, 0), (G_GENO | 1), */
/*         A(ATTK(AT_BITE, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(800, 250, MS_BARK, MZ_MEDIUM), 0, 0, */
/*         M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_STRONG | M2_DOMESTIC, */
/*         M3_INFRAVISIBLE, HI_DOMESTIC), */
/*     MON("wolf", S_DOG, LVL(5, 12, 4, 0, 0), (G_GENO | G_SGROUP | 2), */
/*         A(ATTK(AT_BITE, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(500, 250, MS_BARK, MZ_MEDIUM), 0, 0, */
/*         M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, M3_INFRAVISIBLE, */
/*         CLR_BROWN), */
/*     MON("werewolf", S_DOG, LVL(5, 12, 4, 20, -7), (G_NOGEN | G_NOCORPSE), */
/*         A(ATTK(AT_BITE, AD_WERE, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(500, 250, MS_BARK, MZ_MEDIUM), MR_POISON, 0, */
/*         M1_NOHANDS | M1_POIS | M1_REGEN | M1_CARNIVORE, */
/*         M2_NOPOLY | M2_WERE | M2_HOSTILE, M3_INFRAVISIBLE, CLR_BROWN), */
/*     MON("winter wolf cub", S_DOG, LVL(5, 12, 4, 0, -5), */
/*         (G_NOHELL | G_GENO | G_SGROUP | 2), */
/*         A(ATTK(AT_BITE, AD_PHYS, 1, 8), ATTK(AT_BREA, AD_COLD, 1, 6), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(250, 200, MS_BARK, MZ_SMALL), MR_COLD, MR_COLD, */
/*         M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, 0, CLR_CYAN), */
/*     MON("warg", S_DOG, LVL(7, 12, 4, 0, -5), (G_GENO | G_SGROUP | 2), */
/*         A(ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(850, 350, MS_BARK, MZ_MEDIUM), 0, 0, */
/*         M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, M3_INFRAVISIBLE, */
/*         CLR_BROWN), */
/*     MON("winter wolf", S_DOG, LVL(7, 12, 4, 20, 0), (G_NOHELL | G_GENO | 1), */
/*         A(ATTK(AT_BITE, AD_PHYS, 2, 6), ATTK(AT_BREA, AD_COLD, 2, 6), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(700, 300, MS_BARK, MZ_LARGE), MR_COLD, MR_COLD, */
/*         M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE | M2_STRONG, 0, */
/*         CLR_CYAN), */
/*     MON("hell hound pup", S_DOG, LVL(7, 12, 4, 20, -5), */
/*         (G_HELL | G_GENO | G_SGROUP | 1), */
/*         A(ATTK(AT_BITE, AD_PHYS, 2, 6), ATTK(AT_BREA, AD_FIRE, 2, 6), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(200, 200, MS_BARK, MZ_SMALL), MR_FIRE, MR_FIRE, */
/*         M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, M3_INFRAVISIBLE, */
/*         CLR_RED), */
/*     MON("hell hound", S_DOG, LVL(12, 14, 2, 20, 0), (G_HELL | G_GENO | 1), */
/*         A(ATTK(AT_BITE, AD_PHYS, 3, 6), ATTK(AT_BREA, AD_FIRE, 3, 6), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(600, 300, MS_BARK, MZ_MEDIUM), MR_FIRE, MR_FIRE, */
/*         M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE | M2_STRONG, */
/*         M3_INFRAVISIBLE, CLR_RED), */
/* #ifdef CHARON */
/*     MON("Cerberus", S_DOG, LVL(12, 10, 2, 20, -7), */
/*         (G_NOGEN | G_UNIQ | G_HELL), */
/*         A(ATTK(AT_BITE, AD_PHYS, 3, 6), ATTK(AT_BITE, AD_PHYS, 3, 6), */
/*           ATTK(AT_BITE, AD_PHYS, 3, 6), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1000, 350, MS_BARK, MZ_LARGE), MR_FIRE, MR_FIRE, */
/*         M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, */
/*         M2_NOPOLY | M2_HOSTILE | M2_STRONG | M2_PNAME | M2_MALE, */
/*         M3_INFRAVISIBLE, CLR_RED), */
/* #endif */
/*     #<{(| */
/*      * eyes */
/*      |)}># */
/*     MON("gas spore", S_EYE, LVL(1, 3, 10, 0, 0), (G_NOCORPSE | G_GENO | 1), */
/*         A(ATTK(AT_BOOM, AD_PHYS, 4, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(10, 10, MS_SILENT, MZ_SMALL), 0, 0, */
/*         M1_FLY | M1_BREATHLESS | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS, */
/*         M2_HOSTILE | M2_NEUTER, 0, CLR_GRAY), */
/*     MON("floating eye", S_EYE, LVL(2, 1, 9, 10, 0), (G_GENO | 5), */
/*         A(ATTK(AT_NONE, AD_PLYS, 0, 70), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(10, 10, MS_SILENT, MZ_SMALL), 0, 0, */
/*         M1_FLY | M1_AMPHIBIOUS | M1_NOLIMBS | M1_NOHEAD | M1_NOTAKE, */
/*         M2_HOSTILE | M2_NEUTER, M3_INFRAVISIBLE, CLR_BLUE), */
/*     MON("freezing sphere", S_EYE, LVL(6, 13, 4, 0, 0), */
/*         (G_NOCORPSE | G_NOHELL | G_GENO | 2), */
/*         A(ATTK(AT_EXPL, AD_COLD, 4, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(10, 10, MS_SILENT, MZ_SMALL), MR_COLD, MR_COLD, */
/*         M1_FLY | M1_BREATHLESS | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS */
/*             | M1_NOTAKE, */
/*         M2_HOSTILE | M2_NEUTER, M3_INFRAVISIBLE, CLR_WHITE), */
/*     MON("flaming sphere", S_EYE, LVL(6, 13, 4, 0, 0), */
/*         (G_NOCORPSE | G_GENO | 2), A(ATTK(AT_EXPL, AD_FIRE, 4, 6), NO_ATTK, */
/*                                      NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(10, 10, MS_SILENT, MZ_SMALL), MR_FIRE, MR_FIRE, */
/*         M1_FLY | M1_BREATHLESS | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS */
/*             | M1_NOTAKE, */
/*         M2_HOSTILE | M2_NEUTER, M3_INFRAVISIBLE, CLR_RED), */
/*     MON("shocking sphere", S_EYE, LVL(6, 13, 4, 0, 0), */
/*         (G_NOCORPSE | G_GENO | 2), A(ATTK(AT_EXPL, AD_ELEC, 4, 6), NO_ATTK, */
/*                                      NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(10, 10, MS_SILENT, MZ_SMALL), MR_ELEC, MR_ELEC, */
/*         M1_FLY | M1_BREATHLESS | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS */
/*             | M1_NOTAKE, */
/*         M2_HOSTILE | M2_NEUTER, M3_INFRAVISIBLE, HI_ZAP), */
/* #if 0 #<{(| not yet implemented |)}># */
/*     MON("beholder", S_EYE, */
/*         LVL(6, 3, 4, 0, -10), (G_GENO | 2), */
/*         A(ATTK(AT_GAZE, AD_SLOW, 0, 0), ATTK(AT_GAZE, AD_SLEE, 2,25), */
/*           ATTK(AT_GAZE, AD_DISN, 0, 0), ATTK(AT_GAZE, AD_STON, 0, 0), */
/*           ATTK(AT_GAZE, AD_CNCL, 2, 4), ATTK(AT_BITE, AD_PHYS, 2, 4)), */
/*         SIZ(10, 10, MS_SILENT, MZ_SMALL), MR_COLD, 0, */
/*         M1_FLY | M1_BREATHLESS | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS, */
/*         M2_NOPOLY | M2_HOSTILE | M2_NEUTER, M3_INFRAVISIBLE, CLR_BROWN), */
/* #endif */
/*     #<{(| */
/*      * felines */
/*      |)}># */
/*     MON("kitten", S_FELINE, LVL(2, 18, 6, 0, 0), (G_GENO | 1), */
/*         A(ATTK(AT_BITE, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(150, 150, MS_MEW, MZ_SMALL), 0, 0, */
/*         M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_WANDER | M2_DOMESTIC, */
/*         M3_INFRAVISIBLE, HI_DOMESTIC), */
/*     MON("housecat", S_FELINE, LVL(4, 16, 5, 0, 0), (G_GENO | 1), */
/*         A(ATTK(AT_BITE, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(200, 200, MS_MEW, MZ_SMALL), 0, 0, */
/*         M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_DOMESTIC, M3_INFRAVISIBLE, */
/*         HI_DOMESTIC), */
/*     MON("jaguar", S_FELINE, LVL(4, 15, 6, 0, 0), (G_GENO | 2), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4), */
/*           ATTK(AT_BITE, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(600, 300, MS_GROWL, MZ_LARGE), 0, 0, */
/*         M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, M3_INFRAVISIBLE, */
/*         CLR_BROWN), */
/*     MON("lynx", S_FELINE, LVL(5, 15, 6, 0, 0), (G_GENO | 1), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4), */
/*           ATTK(AT_BITE, AD_PHYS, 1, 10), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(600, 300, MS_GROWL, MZ_SMALL), 0, 0, */
/*         M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, M3_INFRAVISIBLE, */
/*         CLR_CYAN), */
/*     MON("panther", S_FELINE, LVL(5, 15, 6, 0, 0), (G_GENO | 1), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 1, 6), ATTK(AT_CLAW, AD_PHYS, 1, 6), */
/*           ATTK(AT_BITE, AD_PHYS, 1, 10), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(600, 300, MS_GROWL, MZ_LARGE), 0, 0, */
/*         M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, M3_INFRAVISIBLE, */
/*         CLR_BLACK), */
/*     MON("large cat", S_FELINE, LVL(6, 15, 4, 0, 0), (G_GENO | 1), */
/*         A(ATTK(AT_BITE, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(250, 250, MS_MEW, MZ_SMALL), 0, 0, */
/*         M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_STRONG | M2_DOMESTIC, */
/*         M3_INFRAVISIBLE, HI_DOMESTIC), */
/*     MON("tiger", S_FELINE, LVL(6, 12, 6, 0, 0), (G_GENO | 2), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 2, 4), ATTK(AT_CLAW, AD_PHYS, 2, 4), */
/*           ATTK(AT_BITE, AD_PHYS, 1, 10), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(600, 300, MS_GROWL, MZ_LARGE), 0, 0, */
/*         M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, M3_INFRAVISIBLE, */
/*         CLR_YELLOW), */
/*     #<{(| */
/*      * gremlins and gargoyles */
/*      |)}># */
/*     MON("gremlin", S_GREMLIN, LVL(5, 12, 2, 25, -9), (G_GENO | 2), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 1, 6), ATTK(AT_CLAW, AD_PHYS, 1, 6), */
/*           ATTK(AT_BITE, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_CURS, 0, 0), NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(100, 20, MS_LAUGH, MZ_SMALL), MR_POISON, MR_POISON, */
/*         M1_SWIM | M1_HUMANOID | M1_POIS, M2_STALK, M3_INFRAVISIBLE, */
/*         CLR_GREEN), */
/*     MON("gargoyle", S_GREMLIN, LVL(6, 10, -4, 0, -9), (G_GENO | 2), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 2, 6), ATTK(AT_CLAW, AD_PHYS, 2, 6), */
/*           ATTK(AT_BITE, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1000, 200, MS_GRUNT, MZ_HUMAN), MR_STONE, MR_STONE, */
/*         M1_HUMANOID | M1_THICK_HIDE | M1_BREATHLESS, M2_HOSTILE | M2_STRONG, */
/*         0, CLR_BROWN), */
/*     MON("winged gargoyle", S_GREMLIN, LVL(9, 15, -2, 0, -12), (G_GENO | 1), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 3, 6), ATTK(AT_CLAW, AD_PHYS, 3, 6), */
/*           ATTK(AT_BITE, AD_PHYS, 3, 4), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1200, 300, MS_GRUNT, MZ_HUMAN), MR_STONE, MR_STONE, */
/*         M1_FLY | M1_HUMANOID | M1_THICK_HIDE | M1_BREATHLESS | M1_OVIPAROUS, */
/*         M2_LORD | M2_HOSTILE | M2_STRONG | M2_MAGIC, 0, HI_LORD), */
/*     #<{(| */
/*      * humanoids */
/*      |)}># */
/*     MON("hobbit", S_HUMANOID, LVL(1, 9, 10, 0, 6), (G_GENO | 2), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(500, 200, MS_HUMANOID, MZ_SMALL), 0, 0, M1_HUMANOID | M1_OMNIVORE, */
/*         M2_COLLECT, M3_INFRAVISIBLE | M3_INFRAVISION, CLR_GREEN), */
/*     MON("dwarf", S_HUMANOID, LVL(2, 6, 10, 10, 4), (G_GENO | 3), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(900, 300, MS_HUMANOID, MZ_HUMAN), 0, 0, */
/*         M1_TUNNEL | M1_NEEDPICK | M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_DWARF | M2_STRONG | M2_GREEDY | M2_JEWELS | M2_COLLECT, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED), */
/*     MON("bugbear", S_HUMANOID, LVL(3, 9, 5, 0, -6), (G_GENO | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(1250, 250, MS_GROWL, MZ_LARGE), 0, 0, M1_HUMANOID | M1_OMNIVORE, */
/*         M2_STRONG | M2_COLLECT, M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BROWN), */
/*     MON("dwarf lord", S_HUMANOID, LVL(4, 6, 10, 10, 5), (G_GENO | 2), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 4), ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(900, 300, MS_HUMANOID, MZ_HUMAN), 0, 0, */
/*         M1_TUNNEL | M1_NEEDPICK | M1_HUMANOID | M1_OMNIVORE, */
/*         M2_DWARF | M2_STRONG | M2_LORD | M2_MALE | M2_GREEDY | M2_JEWELS */
/*             | M2_COLLECT, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BLUE), */
/*     MON("dwarf king", S_HUMANOID, LVL(6, 6, 10, 20, 6), (G_GENO | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 6), ATTK(AT_WEAP, AD_PHYS, 2, 6), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(900, 300, MS_HUMANOID, MZ_HUMAN), 0, 0, */
/*         M1_TUNNEL | M1_NEEDPICK | M1_HUMANOID | M1_OMNIVORE, */
/*         M2_DWARF | M2_STRONG | M2_PRINCE | M2_MALE | M2_GREEDY | M2_JEWELS */
/*             | M2_COLLECT, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, HI_LORD), */
/*     MON("mind flayer", S_HUMANOID, LVL(9, 12, 5, 90, -8), (G_GENO | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 4), ATTK(AT_TENT, AD_DRIN, 2, 1), */
/*           ATTK(AT_TENT, AD_DRIN, 2, 1), ATTK(AT_TENT, AD_DRIN, 2, 1), NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(1450, 400, MS_HISS, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_FLY | M1_SEE_INVIS | M1_OMNIVORE, */
/*         M2_HOSTILE | M2_NASTY | M2_GREEDY | M2_JEWELS | M2_COLLECT, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_MAGENTA), */
/*     MON("master mind flayer", S_HUMANOID, LVL(13, 12, 0, 90, -8), */
/*         (G_GENO | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 8), ATTK(AT_TENT, AD_DRIN, 2, 1), */
/*           ATTK(AT_TENT, AD_DRIN, 2, 1), ATTK(AT_TENT, AD_DRIN, 2, 1), */
/*           ATTK(AT_TENT, AD_DRIN, 2, 1), ATTK(AT_TENT, AD_DRIN, 2, 1)), */
/*         SIZ(1450, 400, MS_HISS, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_FLY | M1_SEE_INVIS | M1_OMNIVORE, */
/*         M2_HOSTILE | M2_NASTY | M2_GREEDY | M2_JEWELS | M2_COLLECT, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_MAGENTA), */
/*     #<{(| */
/*      * imps & other minor demons/devils */
/*      |)}># */
/*     MON("manes", S_IMP, LVL(1, 3, 7, 0, -7), */
/*         (G_GENO | G_LGROUP | G_NOCORPSE | 1), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 1, 3), ATTK(AT_CLAW, AD_PHYS, 1, 3), */
/*           ATTK(AT_BITE, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(100, 100, MS_SILENT, MZ_SMALL), MR_SLEEP | MR_POISON, 0, M1_POIS, */
/*         M2_HOSTILE | M2_STALK, M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED), */
/*     MON("homunculus", S_IMP, LVL(2, 12, 6, 10, -7), (G_GENO | 2), */
/*         A(ATTK(AT_BITE, AD_SLEE, 1, 3), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(60, 100, MS_SILENT, MZ_TINY), MR_SLEEP | MR_POISON, */
/*         MR_SLEEP | MR_POISON, M1_FLY | M1_POIS, M2_STALK, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_GREEN), */
/*     MON("imp", S_IMP, LVL(3, 12, 2, 20, -7), (G_GENO | 1), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(20, 10, MS_CUSS, MZ_TINY), 0, 0, M1_REGEN, M2_WANDER | M2_STALK, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED), */
/*     MON("lemure", S_IMP, LVL(3, 3, 7, 0, -7), */
/*         (G_HELL | G_GENO | G_LGROUP | G_NOCORPSE | 1), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 1, 3), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(150, 100, MS_SILENT, MZ_MEDIUM), MR_SLEEP | MR_POISON, MR_SLEEP, */
/*         M1_POIS | M1_REGEN, M2_HOSTILE | M2_WANDER | M2_STALK | M2_NEUTER, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BROWN), */
/*     MON("quasit", S_IMP, LVL(3, 15, 2, 20, -7), (G_GENO | 2), */
/*         A(ATTK(AT_CLAW, AD_DRDX, 1, 2), ATTK(AT_CLAW, AD_DRDX, 1, 2), */
/*           ATTK(AT_BITE, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(200, 200, MS_SILENT, MZ_SMALL), MR_POISON, MR_POISON, M1_REGEN, */
/*         M2_STALK, M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BLUE), */
/*     MON("tengu", S_IMP, LVL(6, 13, 5, 30, 7), (G_GENO | 3), */
/*         A(ATTK(AT_BITE, AD_PHYS, 1, 7), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(300, 200, MS_SQAWK, MZ_SMALL), MR_POISON, MR_POISON, */
/*         M1_TPORT | M1_TPORT_CNTRL, M2_STALK, M3_INFRAVISIBLE | M3_INFRAVISION, */
/*         CLR_CYAN), */
/*     #<{(| */
/*      * jellies */
/*      |)}># */
/*     MON("blue jelly", S_JELLY, LVL(4, 0, 8, 10, 0), (G_GENO | 2), */
/*         A(ATTK(AT_NONE, AD_COLD, 0, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(50, 20, MS_SILENT, MZ_MEDIUM), MR_COLD | MR_POISON, */
/*         MR_COLD | MR_POISON, */
/*         M1_BREATHLESS | M1_AMORPHOUS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD */
/*             | M1_MINDLESS | M1_NOTAKE, */
/*         M2_HOSTILE | M2_NEUTER, 0, CLR_BLUE), */
/*     MON("spotted jelly", S_JELLY, LVL(5, 0, 8, 10, 0), (G_GENO | 1), */
/*         A(ATTK(AT_NONE, AD_ACID, 0, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(50, 20, MS_SILENT, MZ_MEDIUM), MR_ACID | MR_STONE, 0, */
/*         M1_BREATHLESS | M1_AMORPHOUS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD */
/*             | M1_MINDLESS | M1_ACID | M1_NOTAKE, */
/*         M2_HOSTILE | M2_NEUTER, 0, CLR_GREEN), */
/*     MON("ochre jelly", S_JELLY, LVL(6, 3, 8, 20, 0), (G_GENO | 2), */
/*         A(ATTK(AT_ENGL, AD_ACID, 3, 6), ATTK(AT_NONE, AD_ACID, 3, 6), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(50, 20, MS_SILENT, MZ_MEDIUM), MR_ACID | MR_STONE, 0, */
/*         M1_BREATHLESS | M1_AMORPHOUS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD */
/*             | M1_MINDLESS | M1_ACID | M1_NOTAKE, */
/*         M2_HOSTILE | M2_NEUTER, 0, CLR_BROWN), */
/*     #<{(| */
/*      * kobolds */
/*      |)}># */
/*     MON("kobold", S_KOBOLD, LVL(0, 6, 10, 0, -2), (G_GENO | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(400, 100, MS_ORC, MZ_SMALL), MR_POISON, 0, */
/*         M1_HUMANOID | M1_POIS | M1_OMNIVORE, M2_HOSTILE | M2_COLLECT, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BROWN), */
/*     MON("large kobold", S_KOBOLD, LVL(1, 6, 10, 0, -3), (G_GENO | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(450, 150, MS_ORC, MZ_SMALL), MR_POISON, 0, */
/*         M1_HUMANOID | M1_POIS | M1_OMNIVORE, M2_HOSTILE | M2_COLLECT, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED), */
/*     MON("kobold lord", S_KOBOLD, LVL(2, 6, 10, 0, -4), (G_GENO | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(500, 200, MS_ORC, MZ_SMALL), MR_POISON, 0, */
/*         M1_HUMANOID | M1_POIS | M1_OMNIVORE, */
/*         M2_HOSTILE | M2_LORD | M2_MALE | M2_COLLECT, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, HI_LORD), */
/*     MON("kobold shaman", S_KOBOLD, LVL(2, 6, 6, 10, -4), (G_GENO | 1), */
/*         A(ATTK(AT_MAGC, AD_SPEL, 0, 0), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(450, 150, MS_ORC, MZ_SMALL), MR_POISON, 0, */
/*         M1_HUMANOID | M1_POIS | M1_OMNIVORE, M2_HOSTILE | M2_MAGIC, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, HI_ZAP), */
/*     #<{(| */
/*      * leprechauns */
/*      |)}># */
/*     MON("leprechaun", S_LEPRECHAUN, LVL(5, 15, 8, 20, 0), (G_GENO | 4), */
/*         A(ATTK(AT_CLAW, AD_SGLD, 1, 2), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(60, 30, MS_LAUGH, MZ_TINY), 0, 0, M1_HUMANOID | M1_TPORT, */
/*         M2_HOSTILE | M2_GREEDY, M3_INFRAVISIBLE, CLR_GREEN), */
/*     #<{(| */
/*      * mimics */
/*      |)}># */
/*     MON("small mimic", S_MIMIC, LVL(7, 3, 7, 0, 0), (G_GENO | 2), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 3, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(300, 200, MS_SILENT, MZ_MEDIUM), MR_ACID, 0, */
/*         M1_BREATHLESS | M1_AMORPHOUS | M1_HIDE | M1_ANIMAL | M1_NOEYES */
/*             | M1_NOHEAD | M1_NOLIMBS | M1_THICK_HIDE | M1_CARNIVORE, */
/*         M2_HOSTILE, 0, CLR_BROWN), */
/*     MON("large mimic", S_MIMIC, LVL(8, 3, 7, 10, 0), (G_GENO | 1), */
/*         A(ATTK(AT_CLAW, AD_STCK, 3, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(600, 400, MS_SILENT, MZ_LARGE), MR_ACID, 0, */
/*         M1_CLING | M1_BREATHLESS | M1_AMORPHOUS | M1_HIDE | M1_ANIMAL */
/*             | M1_NOEYES | M1_NOHEAD | M1_NOLIMBS | M1_THICK_HIDE */
/*             | M1_CARNIVORE, */
/*         M2_HOSTILE | M2_STRONG, 0, CLR_RED), */
/*     MON("giant mimic", S_MIMIC, LVL(9, 3, 7, 20, 0), (G_GENO | 1), */
/*         A(ATTK(AT_CLAW, AD_STCK, 3, 6), ATTK(AT_CLAW, AD_STCK, 3, 6), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(800, 500, MS_SILENT, MZ_LARGE), MR_ACID, 0, */
/*         M1_CLING | M1_BREATHLESS | M1_AMORPHOUS | M1_HIDE | M1_ANIMAL */
/*             | M1_NOEYES | M1_NOHEAD | M1_NOLIMBS | M1_THICK_HIDE */
/*             | M1_CARNIVORE, */
/*         M2_HOSTILE | M2_STRONG, 0, HI_LORD), */
/*     #<{(| */
/*      * nymphs */
/*      |)}># */
/*     MON("wood nymph", S_NYMPH, LVL(3, 12, 9, 20, 0), (G_GENO | 2), */
/*         A(ATTK(AT_CLAW, AD_SITM, 0, 0), ATTK(AT_CLAW, AD_SEDU, 0, 0), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(600, 300, MS_SEDUCE, MZ_HUMAN), 0, 0, M1_HUMANOID | M1_TPORT, */
/*         M2_HOSTILE | M2_FEMALE | M2_COLLECT, M3_INFRAVISIBLE, CLR_GREEN), */
/*     MON("water nymph", S_NYMPH, LVL(3, 12, 9, 20, 0), (G_GENO | 2), */
/*         A(ATTK(AT_CLAW, AD_SITM, 0, 0), ATTK(AT_CLAW, AD_SEDU, 0, 0), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(600, 300, MS_SEDUCE, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_TPORT | M1_SWIM, M2_HOSTILE | M2_FEMALE | M2_COLLECT, */
/*         M3_INFRAVISIBLE, CLR_BLUE), */
/*     MON("mountain nymph", S_NYMPH, LVL(3, 12, 9, 20, 0), (G_GENO | 2), */
/*         A(ATTK(AT_CLAW, AD_SITM, 0, 0), ATTK(AT_CLAW, AD_SEDU, 0, 0), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(600, 300, MS_SEDUCE, MZ_HUMAN), 0, 0, M1_HUMANOID | M1_TPORT, */
/*         M2_HOSTILE | M2_FEMALE | M2_COLLECT, M3_INFRAVISIBLE, CLR_BROWN), */
/*     #<{(| */
/*      * orcs */
/*      |)}># */
/*     MON("goblin", S_ORC, LVL(0, 6, 10, 0, -3), (G_GENO | 2), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(400, 100, MS_ORC, MZ_SMALL), 0, 0, M1_HUMANOID | M1_OMNIVORE, */
/*         M2_ORC | M2_COLLECT, M3_INFRAVISIBLE | M3_INFRAVISION, CLR_GRAY), */
/*     MON("hobgoblin", S_ORC, LVL(1, 9, 10, 0, -4), (G_GENO | 2), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(1000, 200, MS_ORC, MZ_HUMAN), 0, 0, M1_HUMANOID | M1_OMNIVORE, */
/*         M2_ORC | M2_STRONG | M2_COLLECT, M3_INFRAVISIBLE | M3_INFRAVISION, */
/*         CLR_BROWN), */
/*     #<{(| plain "orc" for zombie corpses only; not created at random */
/*      |)}># */
/*     MON("orc", S_ORC, LVL(1, 9, 10, 0, -3), (G_GENO | G_NOGEN | G_LGROUP), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(850, 150, MS_ORC, MZ_HUMAN), 0, 0, M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_ORC | M2_STRONG | M2_GREEDY | M2_JEWELS | M2_COLLECT, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED), */
/*     MON("hill orc", S_ORC, LVL(2, 9, 10, 0, -4), (G_GENO | G_LGROUP | 2), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(1000, 200, MS_ORC, MZ_HUMAN), 0, 0, M1_HUMANOID | M1_OMNIVORE, */
/*         M2_ORC | M2_STRONG | M2_GREEDY | M2_JEWELS | M2_COLLECT, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_YELLOW), */
/*     MON("Mordor orc", S_ORC, LVL(3, 5, 10, 0, -5), (G_GENO | G_LGROUP | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(1200, 200, MS_ORC, MZ_HUMAN), 0, 0, M1_HUMANOID | M1_OMNIVORE, */
/*         M2_ORC | M2_STRONG | M2_GREEDY | M2_JEWELS | M2_COLLECT, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BLUE), */
/*     MON("Uruk-hai", S_ORC, LVL(3, 7, 10, 0, -4), (G_GENO | G_LGROUP | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(1300, 300, MS_ORC, MZ_HUMAN), 0, 0, M1_HUMANOID | M1_OMNIVORE, */
/*         M2_ORC | M2_STRONG | M2_GREEDY | M2_JEWELS | M2_COLLECT, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BLACK), */
/*     MON("orc shaman", S_ORC, LVL(3, 9, 5, 10, -5), (G_GENO | 1), */
/*         A(ATTK(AT_MAGC, AD_SPEL, 0, 0), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(1000, 300, MS_ORC, MZ_HUMAN), 0, 0, M1_HUMANOID | M1_OMNIVORE, */
/*         M2_ORC | M2_STRONG | M2_GREEDY | M2_JEWELS | M2_MAGIC, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, HI_ZAP), */
/*     MON("orc-captain", S_ORC, LVL(5, 5, 10, 0, -5), (G_GENO | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 4), ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1350, 350, MS_ORC, MZ_HUMAN), 0, 0, M1_HUMANOID | M1_OMNIVORE, */
/*         M2_ORC | M2_STRONG | M2_GREEDY | M2_JEWELS | M2_COLLECT, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, HI_LORD), */
/*     #<{(| */
/*      * piercers */
/*      |)}># */
/*     MON("rock piercer", S_PIERCER, LVL(3, 1, 3, 0, 0), (G_GENO | 4), */
/*         A(ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(200, 200, MS_SILENT, MZ_SMALL), 0, 0, */
/*         M1_CLING | M1_HIDE | M1_ANIMAL | M1_NOEYES | M1_NOLIMBS | M1_CARNIVORE */
/*             | M1_NOTAKE, */
/*         M2_HOSTILE, 0, CLR_GRAY), */
/*     MON("iron piercer", S_PIERCER, LVL(5, 1, 0, 0, 0), (G_GENO | 2), */
/*         A(ATTK(AT_BITE, AD_PHYS, 3, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(400, 300, MS_SILENT, MZ_MEDIUM), 0, 0, */
/*         M1_CLING | M1_HIDE | M1_ANIMAL | M1_NOEYES | M1_NOLIMBS | M1_CARNIVORE */
/*             | M1_NOTAKE, */
/*         M2_HOSTILE, 0, CLR_CYAN), */
/*     MON("glass piercer", S_PIERCER, LVL(7, 1, 0, 0, 0), (G_GENO | 1), */
/*         A(ATTK(AT_BITE, AD_PHYS, 4, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(400, 300, MS_SILENT, MZ_MEDIUM), MR_ACID, 0, */
/*         M1_CLING | M1_HIDE | M1_ANIMAL | M1_NOEYES | M1_NOLIMBS | M1_CARNIVORE */
/*             | M1_NOTAKE, */
/*         M2_HOSTILE, 0, CLR_WHITE), */
/*     #<{(| */
/*      * quadrupeds */
/*      |)}># */
/*     MON("rothe", S_QUADRUPED, LVL(2, 9, 7, 0, 0), (G_GENO | G_SGROUP | 4), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 1, 3), ATTK(AT_BITE, AD_PHYS, 1, 3), */
/*           ATTK(AT_BITE, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(400, 100, MS_SILENT, MZ_LARGE), 0, 0, */
/*         M1_ANIMAL | M1_NOHANDS | M1_OMNIVORE, M2_HOSTILE, M3_INFRAVISIBLE, */
/*         CLR_BROWN), */
/*     MON("mumak", S_QUADRUPED, LVL(5, 9, 0, 0, -2), (G_GENO | 1), */
/*         A(ATTK(AT_BUTT, AD_PHYS, 4, 12), ATTK(AT_BITE, AD_PHYS, 2, 6), */
/*           NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(2500, 500, MS_ROAR, MZ_LARGE), 0, 0, */
/*         M1_ANIMAL | M1_THICK_HIDE | M1_NOHANDS | M1_HERBIVORE, */
/*         M2_HOSTILE | M2_STRONG, M3_INFRAVISIBLE, CLR_GRAY), */
/*     MON("leocrotta", S_QUADRUPED, LVL(6, 18, 4, 10, 0), (G_GENO | 2), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 2, 6), ATTK(AT_BITE, AD_PHYS, 2, 6), */
/*           ATTK(AT_CLAW, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1200, 500, MS_IMITATE, MZ_LARGE), 0, 0, */
/*         M1_ANIMAL | M1_NOHANDS | M1_OMNIVORE, M2_HOSTILE | M2_STRONG, */
/*         M3_INFRAVISIBLE, CLR_RED), */
/*     MON("wumpus", S_QUADRUPED, LVL(8, 3, 2, 10, 0), (G_GENO | 1), */
/*         A(ATTK(AT_BITE, AD_PHYS, 3, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(2500, 500, MS_BURBLE, MZ_LARGE), 0, 0, */
/*         M1_CLING | M1_ANIMAL | M1_NOHANDS | M1_OMNIVORE, */
/*         M2_HOSTILE | M2_STRONG, M3_INFRAVISIBLE, CLR_CYAN), */
/*     MON("titanothere", S_QUADRUPED, LVL(12, 12, 6, 0, 0), (G_GENO | 2), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 2, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(2650, 650, MS_SILENT, MZ_LARGE), 0, 0, */
/*         M1_ANIMAL | M1_THICK_HIDE | M1_NOHANDS | M1_HERBIVORE, */
/*         M2_HOSTILE | M2_STRONG, M3_INFRAVISIBLE, CLR_GRAY), */
/*     MON("baluchitherium", S_QUADRUPED, LVL(14, 12, 5, 0, 0), (G_GENO | 2), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 5, 4), ATTK(AT_CLAW, AD_PHYS, 5, 4), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(3800, 800, MS_SILENT, MZ_LARGE), 0, 0, */
/*         M1_ANIMAL | M1_THICK_HIDE | M1_NOHANDS | M1_HERBIVORE, */
/*         M2_HOSTILE | M2_STRONG, M3_INFRAVISIBLE, CLR_GRAY), */
/*     MON("mastodon", S_QUADRUPED, LVL(20, 12, 5, 0, 0), (G_GENO | 1), */
/*         A(ATTK(AT_BUTT, AD_PHYS, 4, 8), ATTK(AT_BUTT, AD_PHYS, 4, 8), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(3800, 800, MS_SILENT, MZ_LARGE), 0, 0, */
/*         M1_ANIMAL | M1_THICK_HIDE | M1_NOHANDS | M1_HERBIVORE, */
/*         M2_HOSTILE | M2_STRONG, M3_INFRAVISIBLE, CLR_BLACK), */
/*     #<{(| */
/*      * rodents */
/*      |)}># */
/*     MON("sewer rat", S_RODENT, LVL(0, 12, 7, 0, 0), (G_GENO | G_SGROUP | 1), */
/*         A(ATTK(AT_BITE, AD_PHYS, 1, 3), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(20, 12, MS_SQEEK, MZ_TINY), 0, 0, */
/*         M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, M3_INFRAVISIBLE, */
/*         CLR_BROWN), */
/*     MON("giant rat", S_RODENT, LVL(1, 10, 7, 0, 0), (G_GENO | G_SGROUP | 2), */
/*         A(ATTK(AT_BITE, AD_PHYS, 1, 3), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(30, 30, MS_SQEEK, MZ_TINY), 0, 0, */
/*         M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, M3_INFRAVISIBLE, */
/*         CLR_BROWN), */
/*     MON("rabid rat", S_RODENT, LVL(2, 12, 6, 0, 0), (G_GENO | 1), */
/*         A(ATTK(AT_BITE, AD_DRCO, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(30, 5, MS_SQEEK, MZ_TINY), MR_POISON, 0, */
/*         M1_ANIMAL | M1_NOHANDS | M1_POIS | M1_CARNIVORE, M2_HOSTILE, */
/*         M3_INFRAVISIBLE, CLR_BROWN), */
/*     MON("wererat", S_RODENT, LVL(2, 12, 6, 10, -7), (G_NOGEN | G_NOCORPSE), */
/*         A(ATTK(AT_BITE, AD_WERE, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(40, 30, MS_SQEEK, MZ_TINY), MR_POISON, 0, */
/*         M1_NOHANDS | M1_POIS | M1_REGEN | M1_CARNIVORE, */
/*         M2_NOPOLY | M2_WERE | M2_HOSTILE, M3_INFRAVISIBLE, CLR_BROWN), */
/*     MON("rock mole", S_RODENT, LVL(3, 3, 0, 20, 0), (G_GENO | 2), */
/*         A(ATTK(AT_BITE, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(30, 30, MS_SILENT, MZ_SMALL), 0, 0, */
/*         M1_TUNNEL | M1_ANIMAL | M1_NOHANDS | M1_METALLIVORE, */
/*         M2_HOSTILE | M2_GREEDY | M2_JEWELS | M2_COLLECT, M3_INFRAVISIBLE, */
/*         CLR_GRAY), */
/*     MON("woodchuck", S_RODENT, LVL(3, 3, 0, 20, 0), (G_NOGEN | G_GENO), */
/*         A(ATTK(AT_BITE, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(30, 30, MS_SILENT, MZ_SMALL), 0, 0, */
/*         M1_TUNNEL #<{(|LOGGING|)}># | M1_ANIMAL | M1_NOHANDS | M1_SWIM */
/*             | M1_HERBIVORE, */
/*         #<{(| In reality, they tunnel instead of cutting lumber.  Oh, well. |)}># */
/*         M2_WANDER | M2_HOSTILE, M3_INFRAVISIBLE, CLR_BROWN), */
/*     #<{(| */
/*      * spiders & scorpions (keep webmaker() in sync if new critters are added) */
/*      |)}># */
/*     MON("cave spider", S_SPIDER, LVL(1, 12, 3, 0, 0), (G_GENO | G_SGROUP | 2), */
/*         A(ATTK(AT_BITE, AD_PHYS, 1, 2), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(50, 50, MS_SILENT, MZ_TINY), MR_POISON, MR_POISON, */
/*         M1_CONCEAL | M1_ANIMAL | M1_NOHANDS | M1_OVIPAROUS | M1_CARNIVORE, */
/*         M2_HOSTILE, 0, CLR_GRAY), */
/*     MON("centipede", S_SPIDER, LVL(2, 4, 3, 0, 0), (G_GENO | 1), */
/*         A(ATTK(AT_BITE, AD_DRST, 1, 3), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(50, 50, MS_SILENT, MZ_TINY), MR_POISON, MR_POISON, */
/*         M1_CONCEAL | M1_ANIMAL | M1_NOHANDS | M1_OVIPAROUS | M1_CARNIVORE, */
/*         M2_HOSTILE, 0, CLR_YELLOW), */
/*     MON("giant spider", S_SPIDER, LVL(5, 15, 4, 0, 0), (G_GENO | 1), */
/*         A(ATTK(AT_BITE, AD_DRST, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(100, 100, MS_SILENT, MZ_LARGE), MR_POISON, MR_POISON, */
/*         M1_ANIMAL | M1_NOHANDS | M1_OVIPAROUS | M1_POIS | M1_CARNIVORE, */
/*         M2_HOSTILE | M2_STRONG, 0, CLR_MAGENTA), */
/*     MON("scorpion", S_SPIDER, LVL(5, 15, 3, 0, 0), (G_GENO | 2), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 1, 2), ATTK(AT_CLAW, AD_PHYS, 1, 2), */
/*           ATTK(AT_STNG, AD_DRST, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(50, 100, MS_SILENT, MZ_SMALL), MR_POISON, MR_POISON, */
/*         M1_CONCEAL | M1_ANIMAL | M1_NOHANDS | M1_OVIPAROUS | M1_POIS */
/*             | M1_CARNIVORE, */
/*         M2_HOSTILE, 0, CLR_RED), */
/*     #<{(| */
/*      * trappers, lurkers, &c */
/*      |)}># */
/*     MON("lurker above", S_TRAPPER, LVL(10, 3, 3, 0, 0), (G_GENO | 2), */
/*         A(ATTK(AT_ENGL, AD_DGST, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(800, 350, MS_SILENT, MZ_HUGE), 0, 0, */
/*         M1_HIDE | M1_FLY | M1_ANIMAL | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD */
/*             | M1_CARNIVORE, */
/*         M2_HOSTILE | M2_STALK | M2_STRONG, 0, CLR_GRAY), */
/*     MON("trapper", S_TRAPPER, LVL(12, 3, 3, 0, 0), (G_GENO | 2), */
/*         A(ATTK(AT_ENGL, AD_DGST, 1, 10), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(800, 350, MS_SILENT, MZ_HUGE), 0, 0, */
/*         M1_HIDE | M1_ANIMAL | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD */
/*             | M1_CARNIVORE, */
/*         M2_HOSTILE | M2_STALK | M2_STRONG, 0, CLR_GREEN), */
/*     #<{(| */
/*      * unicorns and horses */
/*      |)}># */
/*     MON("pony", S_UNICORN, LVL(3, 16, 6, 0, 0), (G_GENO | 2), */
/*         A(ATTK(AT_KICK, AD_PHYS, 1, 6), ATTK(AT_BITE, AD_PHYS, 1, 2), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1300, 250, MS_NEIGH, MZ_MEDIUM), 0, 0, */
/*         M1_ANIMAL | M1_NOHANDS | M1_HERBIVORE, */
/*         M2_WANDER | M2_STRONG | M2_DOMESTIC, M3_INFRAVISIBLE, CLR_BROWN), */
/*     MON("white unicorn", S_UNICORN, LVL(4, 24, 2, 70, 7), (G_GENO | 2), */
/*         A(ATTK(AT_BUTT, AD_PHYS, 1, 12), ATTK(AT_KICK, AD_PHYS, 1, 6), */
/*           NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1300, 300, MS_NEIGH, MZ_LARGE), MR_POISON, MR_POISON, */
/*         M1_NOHANDS | M1_HERBIVORE, M2_WANDER | M2_STRONG | M2_JEWELS, */
/*         M3_INFRAVISIBLE, CLR_WHITE), */
/*     MON("gray unicorn", S_UNICORN, LVL(4, 24, 2, 70, 0), (G_GENO | 1), */
/*         A(ATTK(AT_BUTT, AD_PHYS, 1, 12), ATTK(AT_KICK, AD_PHYS, 1, 6), */
/*           NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1300, 300, MS_NEIGH, MZ_LARGE), MR_POISON, MR_POISON, */
/*         M1_NOHANDS | M1_HERBIVORE, M2_WANDER | M2_STRONG | M2_JEWELS, */
/*         M3_INFRAVISIBLE, CLR_GRAY), */
/*     MON("black unicorn", S_UNICORN, LVL(4, 24, 2, 70, -7), (G_GENO | 1), */
/*         A(ATTK(AT_BUTT, AD_PHYS, 1, 12), ATTK(AT_KICK, AD_PHYS, 1, 6), */
/*           NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1300, 300, MS_NEIGH, MZ_LARGE), MR_POISON, MR_POISON, */
/*         M1_NOHANDS | M1_HERBIVORE, M2_WANDER | M2_STRONG | M2_JEWELS, */
/*         M3_INFRAVISIBLE, CLR_BLACK), */
/*     MON("horse", S_UNICORN, LVL(5, 20, 5, 0, 0), (G_GENO | 2), */
/*         A(ATTK(AT_KICK, AD_PHYS, 1, 8), ATTK(AT_BITE, AD_PHYS, 1, 3), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1500, 300, MS_NEIGH, MZ_LARGE), 0, 0, */
/*         M1_ANIMAL | M1_NOHANDS | M1_HERBIVORE, */
/*         M2_WANDER | M2_STRONG | M2_DOMESTIC, M3_INFRAVISIBLE, CLR_BROWN), */
/*     MON("warhorse", S_UNICORN, LVL(7, 24, 4, 0, 0), (G_GENO | 2), */
/*         A(ATTK(AT_KICK, AD_PHYS, 1, 10), ATTK(AT_BITE, AD_PHYS, 1, 4), */
/*           NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1800, 350, MS_NEIGH, MZ_LARGE), 0, 0, */
/*         M1_ANIMAL | M1_NOHANDS | M1_HERBIVORE, */
/*         M2_WANDER | M2_STRONG | M2_DOMESTIC, M3_INFRAVISIBLE, CLR_BROWN), */
/*     #<{(| */
/*      * vortices */
/*      |)}># */
/*     MON("fog cloud", S_VORTEX, LVL(3, 1, 0, 0, 0), (G_GENO | G_NOCORPSE | 2), */
/*         A(ATTK(AT_ENGL, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(0, 0, MS_SILENT, MZ_HUGE), MR_SLEEP | MR_POISON | MR_STONE, 0, */
/*         M1_FLY | M1_BREATHLESS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD */
/*             | M1_MINDLESS | M1_AMORPHOUS | M1_UNSOLID, */
/*         M2_HOSTILE | M2_NEUTER, 0, CLR_GRAY), */
/*     MON("dust vortex", S_VORTEX, LVL(4, 20, 2, 30, 0), */
/*         (G_GENO | G_NOCORPSE | 2), A(ATTK(AT_ENGL, AD_BLND, 2, 8), NO_ATTK, */
/*                                      NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(0, 0, MS_SILENT, MZ_HUGE), MR_SLEEP | MR_POISON | MR_STONE, 0, */
/*         M1_FLY | M1_BREATHLESS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD */
/*             | M1_MINDLESS, */
/*         M2_HOSTILE | M2_NEUTER, 0, CLR_BROWN), */
/*     MON("ice vortex", S_VORTEX, LVL(5, 20, 2, 30, 0), */
/*         (G_NOHELL | G_GENO | G_NOCORPSE | 1), */
/*         A(ATTK(AT_ENGL, AD_COLD, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(0, 0, MS_SILENT, MZ_HUGE), */
/*         MR_COLD | MR_SLEEP | MR_POISON | MR_STONE, 0, */
/*         M1_FLY | M1_BREATHLESS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD */
/*             | M1_MINDLESS, */
/*         M2_HOSTILE | M2_NEUTER, M3_INFRAVISIBLE, CLR_CYAN), */
/*     MON("energy vortex", S_VORTEX, LVL(6, 20, 2, 30, 0), */
/*         (G_GENO | G_NOCORPSE | 1), */
/*         A(ATTK(AT_ENGL, AD_ELEC, 1, 6), ATTK(AT_ENGL, AD_DREN, 2, 6), */
/*           ATTK(AT_NONE, AD_ELEC, 0, 4), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(0, 0, MS_SILENT, MZ_HUGE), */
/*         MR_ELEC | MR_SLEEP | MR_DISINT | MR_POISON | MR_STONE, 0, */
/*         M1_FLY | M1_BREATHLESS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD */
/*             | M1_MINDLESS | M1_UNSOLID, */
/*         M2_HOSTILE | M2_NEUTER, 0, HI_ZAP), */
/*     MON("steam vortex", S_VORTEX, LVL(7, 22, 2, 30, 0), */
/*         (G_HELL | G_GENO | G_NOCORPSE | 2), */
/*         A(ATTK(AT_ENGL, AD_FIRE, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(0, 0, MS_SILENT, MZ_HUGE), */
/*         MR_FIRE | MR_SLEEP | MR_POISON | MR_STONE, 0, */
/*         M1_FLY | M1_BREATHLESS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD */
/*             | M1_MINDLESS | M1_UNSOLID, */
/*         M2_HOSTILE | M2_NEUTER, M3_INFRAVISIBLE, CLR_BLUE), */
/*     MON("fire vortex", S_VORTEX, LVL(8, 22, 2, 30, 0), */
/*         (G_HELL | G_GENO | G_NOCORPSE | 1), */
/*         A(ATTK(AT_ENGL, AD_FIRE, 1, 10), ATTK(AT_NONE, AD_FIRE, 0, 4), */
/*           NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(0, 0, MS_SILENT, MZ_HUGE), */
/*         MR_FIRE | MR_SLEEP | MR_POISON | MR_STONE, 0, */
/*         M1_FLY | M1_BREATHLESS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD */
/*             | M1_MINDLESS | M1_UNSOLID, */
/*         M2_HOSTILE | M2_NEUTER, M3_INFRAVISIBLE, CLR_YELLOW), */
/*     #<{(| */
/*      * worms */
/*      |)}># */
/*     MON("baby long worm", S_WORM, LVL(5, 3, 5, 0, 0), G_GENO, */
/*         A(ATTK(AT_BITE, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(600, 250, MS_SILENT, MZ_LARGE), 0, 0, */
/*         M1_ANIMAL | M1_SLITHY | M1_NOLIMBS | M1_CARNIVORE | M1_NOTAKE, */
/*         M2_HOSTILE, 0, CLR_BROWN), */
/*     MON("baby purple worm", S_WORM, LVL(8, 3, 5, 0, 0), G_GENO, */
/*         A(ATTK(AT_BITE, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(600, 250, MS_SILENT, MZ_LARGE), 0, 0, */
/*         M1_ANIMAL | M1_SLITHY | M1_NOLIMBS | M1_CARNIVORE, M2_HOSTILE, 0, */
/*         CLR_MAGENTA), */
/*     MON("long worm", S_WORM, LVL(9, 3, 5, 10, 0), (G_GENO | 2), */
/*         A(ATTK(AT_BITE, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(1500, 500, MS_SILENT, MZ_GIGANTIC), 0, 0, */
/*         M1_ANIMAL | M1_SLITHY | M1_NOLIMBS | M1_OVIPAROUS | M1_CARNIVORE */
/*             | M1_NOTAKE, */
/*         M2_HOSTILE | M2_STRONG | M2_NASTY, 0, CLR_BROWN), */
/*     MON("purple worm", S_WORM, LVL(15, 9, 6, 20, 0), (G_GENO | 2), */
/*         A(ATTK(AT_BITE, AD_PHYS, 2, 8), ATTK(AT_ENGL, AD_DGST, 1, 10), */
/*           NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(2700, 700, MS_SILENT, MZ_GIGANTIC), 0, 0, */
/*         M1_ANIMAL | M1_SLITHY | M1_NOLIMBS | M1_OVIPAROUS | M1_CARNIVORE, */
/*         M2_HOSTILE | M2_STRONG | M2_NASTY, 0, CLR_MAGENTA), */
/*     #<{(| */
/*      * xan, &c */
/*      |)}># */
/*     MON("grid bug", S_XAN, LVL(0, 12, 9, 0, 0), */
/*         (G_GENO | G_SGROUP | G_NOCORPSE | 3), */
/*         A(ATTK(AT_BITE, AD_ELEC, 1, 1), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(15, 10, MS_BUZZ, MZ_TINY), MR_ELEC | MR_POISON, 0, M1_ANIMAL, */
/*         M2_HOSTILE, M3_INFRAVISIBLE, CLR_MAGENTA), */
/*     MON("xan", S_XAN, LVL(7, 18, -4, 0, 0), (G_GENO | 3), */
/*         A(ATTK(AT_STNG, AD_LEGS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(300, 300, MS_BUZZ, MZ_TINY), MR_POISON, MR_POISON, */
/*         M1_FLY | M1_ANIMAL | M1_NOHANDS | M1_POIS, M2_HOSTILE, */
/*         M3_INFRAVISIBLE, CLR_RED), */
/*     #<{(| */
/*      * lights */
/*      |)}># */
/*     MON("yellow light", S_LIGHT, LVL(3, 15, 0, 0, 0), */
/*         (G_NOCORPSE | G_GENO | 4), A(ATTK(AT_EXPL, AD_BLND, 10, 20), NO_ATTK, */
/*                                      NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(0, 0, MS_SILENT, MZ_SMALL), */
/*         MR_FIRE | MR_COLD | MR_ELEC | MR_DISINT | MR_SLEEP | MR_POISON */
/*             | MR_ACID | MR_STONE, */
/*         0, M1_FLY | M1_BREATHLESS | M1_AMORPHOUS | M1_NOEYES | M1_NOLIMBS */
/*                | M1_NOHEAD | M1_MINDLESS | M1_UNSOLID | M1_NOTAKE, */
/*         M2_HOSTILE | M2_NEUTER, M3_INFRAVISIBLE, CLR_YELLOW), */
/*     MON("black light", S_LIGHT, LVL(5, 15, 0, 0, 0), */
/*         (G_NOCORPSE | G_GENO | 2), A(ATTK(AT_EXPL, AD_HALU, 10, 12), NO_ATTK, */
/*                                      NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(0, 0, MS_SILENT, MZ_SMALL), */
/*         MR_FIRE | MR_COLD | MR_ELEC | MR_DISINT | MR_SLEEP | MR_POISON */
/*             | MR_ACID | MR_STONE, */
/*         0, */
/*         M1_FLY | M1_BREATHLESS | M1_AMORPHOUS | M1_NOEYES | M1_NOLIMBS */
/*             | M1_NOHEAD | M1_MINDLESS | M1_UNSOLID | M1_SEE_INVIS | M1_NOTAKE, */
/*         M2_HOSTILE | M2_NEUTER, 0, CLR_BLACK), */
/*     #<{(| */
/*      * zruty */
/*      |)}># */
/*     MON("zruty", S_ZRUTY, LVL(9, 8, 3, 0, 0), (G_GENO | 2), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 3, 4), ATTK(AT_CLAW, AD_PHYS, 3, 4), */
/*           ATTK(AT_BITE, AD_PHYS, 3, 6), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1200, 600, MS_SILENT, MZ_LARGE), 0, 0, */
/*         M1_ANIMAL | M1_HUMANOID | M1_CARNIVORE, M2_HOSTILE | M2_STRONG, */
/*         M3_INFRAVISIBLE, CLR_BROWN), */
/*     #<{(| */
/*      * Angels and other lawful minions */
/*      |)}># */
/*     MON("couatl", S_ANGEL, LVL(8, 10, 5, 30, 7), */
/*         (G_NOHELL | G_SGROUP | G_NOCORPSE | 1), */
/*         A(ATTK(AT_BITE, AD_DRST, 2, 4), ATTK(AT_BITE, AD_PHYS, 1, 3), */
/*           ATTK(AT_HUGS, AD_WRAP, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(900, 400, MS_HISS, MZ_LARGE), MR_POISON, 0, */
/*         M1_FLY | M1_NOHANDS | M1_SLITHY | M1_POIS, */
/*         M2_MINION | M2_STALK | M2_STRONG | M2_NASTY, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_GREEN), */
/*     MON("Aleax", S_ANGEL, LVL(10, 8, 0, 30, 7), (G_NOHELL | G_NOCORPSE | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_WEAP, AD_PHYS, 1, 6), */
/*           ATTK(AT_KICK, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_IMITATE, MZ_HUMAN), */
/*         MR_COLD | MR_ELEC | MR_SLEEP | MR_POISON, 0, */
/*         M1_HUMANOID | M1_SEE_INVIS, */
/*         M2_MINION | M2_STALK | M2_NASTY | M2_COLLECT, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_YELLOW), */
/*     #<{(| Angels start with the emin extension attached, and usually have */
/*        the isminion flag set; however, non-minion Angels can be tamed */
/*        and will switch to edog (guardian Angel is handled specially and */
/*        always sticks with emin) |)}># */
/*     MON("Angel", S_ANGEL, LVL(14, 10, -4, 55, 12), */
/*         (G_NOHELL | G_NOCORPSE | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_WEAP, AD_PHYS, 1, 6), */
/*           ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_MAGC, AD_MAGM, 2, 6), NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_CUSS, MZ_HUMAN), */
/*         MR_COLD | MR_ELEC | MR_SLEEP | MR_POISON, 0, */
/*         M1_FLY | M1_HUMANOID | M1_SEE_INVIS, */
/*         M2_NOPOLY | M2_MINION | M2_STALK | M2_STRONG | M2_NASTY | M2_COLLECT, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_WHITE), */
/*     MON("ki-rin", S_ANGEL, LVL(16, 18, -5, 90, 15), */
/*         (G_NOHELL | G_NOCORPSE | 1), */
/*         A(ATTK(AT_KICK, AD_PHYS, 2, 4), ATTK(AT_KICK, AD_PHYS, 2, 4), */
/*           ATTK(AT_BUTT, AD_PHYS, 3, 6), ATTK(AT_MAGC, AD_SPEL, 2, 6), NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_NEIGH, MZ_LARGE), 0, 0, */
/*         M1_FLY | M1_ANIMAL | M1_NOHANDS | M1_SEE_INVIS, */
/*         M2_NOPOLY | M2_MINION | M2_STALK | M2_STRONG | M2_NASTY | M2_LORD, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, HI_GOLD), */
/*     MON("Archon", S_ANGEL, LVL(19, 16, -6, 80, 15), */
/*         (G_NOHELL | G_NOCORPSE | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 4), ATTK(AT_WEAP, AD_PHYS, 2, 4), */
/*           ATTK(AT_GAZE, AD_BLND, 2, 6), ATTK(AT_CLAW, AD_PHYS, 1, 8), */
/*           ATTK(AT_MAGC, AD_SPEL, 4, 6), NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_CUSS, MZ_LARGE), */
/*         MR_FIRE | MR_COLD | MR_ELEC | MR_SLEEP | MR_POISON, 0, */
/*         M1_FLY | M1_HUMANOID | M1_SEE_INVIS | M1_REGEN, */
/*         M2_NOPOLY | M2_MINION | M2_STALK | M2_STRONG | M2_NASTY | M2_LORD */
/*             | M2_COLLECT | M2_MAGIC, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, HI_LORD), */
/*     #<{(| */
/*      * Bats */
/*      |)}># */
/*     MON("bat", S_BAT, LVL(0, 22, 8, 0, 0), (G_GENO | G_SGROUP | 1), */
/*         A(ATTK(AT_BITE, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(20, 20, MS_SQEEK, MZ_TINY), 0, 0, */
/*         M1_FLY | M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_WANDER, */
/*         M3_INFRAVISIBLE, CLR_BROWN), */
/*     MON("giant bat", S_BAT, LVL(2, 22, 7, 0, 0), (G_GENO | 2), */
/*         A(ATTK(AT_BITE, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(30, 30, MS_SQEEK, MZ_SMALL), 0, 0, */
/*         M1_FLY | M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, */
/*         M2_WANDER | M2_HOSTILE, M3_INFRAVISIBLE, CLR_RED), */
/*     MON("raven", S_BAT, LVL(4, 20, 6, 0, 0), (G_GENO | 2), */
/*         A(ATTK(AT_BITE, AD_PHYS, 1, 6), ATTK(AT_CLAW, AD_BLND, 1, 6), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(40, 20, MS_SQAWK, MZ_SMALL), 0, 0, */
/*         M1_FLY | M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, */
/*         M2_WANDER | M2_HOSTILE, M3_INFRAVISIBLE, CLR_BLACK), */
/*     MON("vampire bat", S_BAT, LVL(5, 20, 6, 0, 0), (G_GENO | 2), */
/*         A(ATTK(AT_BITE, AD_PHYS, 1, 6), ATTK(AT_BITE, AD_DRST, 0, 0), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(30, 20, MS_SQEEK, MZ_SMALL), MR_SLEEP | MR_POISON, 0, */
/*         M1_FLY | M1_ANIMAL | M1_NOHANDS | M1_POIS | M1_REGEN | M1_OMNIVORE, */
/*         M2_HOSTILE, M3_INFRAVISIBLE, CLR_BLACK), */
/*     #<{(| */
/*      * Centaurs */
/*      |)}># */
/*     MON("plains centaur", S_CENTAUR, LVL(4, 18, 4, 0, 0), (G_GENO | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_KICK, AD_PHYS, 1, 6), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(2500, 500, MS_HUMANOID, MZ_LARGE), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, M2_STRONG | M2_GREEDY | M2_COLLECT, */
/*         M3_INFRAVISIBLE, CLR_BROWN), */
/*     MON("forest centaur", S_CENTAUR, LVL(5, 18, 3, 10, -1), (G_GENO | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 8), ATTK(AT_KICK, AD_PHYS, 1, 6), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(2550, 600, MS_HUMANOID, MZ_LARGE), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, M2_STRONG | M2_GREEDY | M2_COLLECT, */
/*         M3_INFRAVISIBLE, CLR_GREEN), */
/*     MON("mountain centaur", S_CENTAUR, LVL(6, 20, 2, 10, -3), (G_GENO | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 10), ATTK(AT_KICK, AD_PHYS, 1, 6), */
/*           ATTK(AT_KICK, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(2550, 500, MS_HUMANOID, MZ_LARGE), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, M2_STRONG | M2_GREEDY | M2_COLLECT, */
/*         M3_INFRAVISIBLE, CLR_CYAN), */
/*     #<{(| */
/*      * Dragons */
/*      |)}># */
/*     #<{(| The order of the dragons is VERY IMPORTANT.  Quite a few */
/*      * pieces of code depend on gray being first and yellow being last. */
/*      * The code also depends on the *order* being the same as that for */
/*      * dragon scale mail and dragon scales in objects.c.  Baby dragons */
/*      * cannot confer intrinsics, to avoid polyself/egg abuse. */
/*      * */
/*      * As reptiles, dragons are cold-blooded and thus aren't seen */
/*      * with infravision.  Red dragons are the exception. */
/*      |)}># */
/*     MON("baby gray dragon", S_DRAGON, LVL(12, 9, 2, 10, 0), G_GENO, */
/*         A(ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(1500, 500, MS_ROAR, MZ_HUGE), 0, 0, */
/*         M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_CARNIVORE, */
/*         M2_HOSTILE | M2_STRONG | M2_GREEDY | M2_JEWELS, 0, CLR_GRAY), */
/*     MON("baby silver dragon", S_DRAGON, LVL(12, 9, 2, 10, 0), G_GENO, */
/*         A(ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(1500, 500, MS_ROAR, MZ_HUGE), 0, 0, */
/*         M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_CARNIVORE, */
/*         M2_HOSTILE | M2_STRONG | M2_GREEDY | M2_JEWELS, 0, DRAGON_SILVER), */
/* #if 0 #<{(| DEFERRED |)}># */
/*     MON("baby shimmering dragon", S_DRAGON, */
/*         LVL(12, 9, 2, 10, 0), G_GENO, */
/*         A(ATTK(AT_BITE, AD_PHYS, 2, 6), */
/*           NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1500, 500, MS_ROAR, MZ_HUGE), 0, 0, */
/*         M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_CARNIVORE, */
/*         M2_HOSTILE | M2_STRONG | M2_GREEDY | M2_JEWELS, 0, CLR_CYAN), */
/* #endif */
/*     MON("baby red dragon", S_DRAGON, LVL(12, 9, 2, 10, 0), G_GENO, */
/*         A(ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(1500, 500, MS_ROAR, MZ_HUGE), MR_FIRE, 0, */
/*         M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_CARNIVORE, */
/*         M2_HOSTILE | M2_STRONG | M2_GREEDY | M2_JEWELS, M3_INFRAVISIBLE, */
/*         CLR_RED), */
/*     MON("baby white dragon", S_DRAGON, LVL(12, 9, 2, 10, 0), G_GENO, */
/*         A(ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(1500, 500, MS_ROAR, MZ_HUGE), MR_COLD, 0, */
/*         M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_CARNIVORE, */
/*         M2_HOSTILE | M2_STRONG | M2_GREEDY | M2_JEWELS, 0, CLR_WHITE), */
/*     MON("baby orange dragon", S_DRAGON, LVL(12, 9, 2, 10, 0), G_GENO, */
/*         A(ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(1500, 500, MS_ROAR, MZ_HUGE), MR_SLEEP, 0, */
/*         M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_CARNIVORE, */
/*         M2_HOSTILE | M2_STRONG | M2_GREEDY | M2_JEWELS, 0, CLR_ORANGE), */
/*     MON("baby black dragon", S_DRAGON, LVL(12, 9, 2, 10, 0), G_GENO, */
/*         A(ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(1500, 500, MS_ROAR, MZ_HUGE), MR_DISINT, 0, */
/*         M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_CARNIVORE, */
/*         M2_HOSTILE | M2_STRONG | M2_GREEDY | M2_JEWELS, 0, CLR_BLACK), */
/*     MON("baby blue dragon", S_DRAGON, LVL(12, 9, 2, 10, 0), G_GENO, */
/*         A(ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(1500, 500, MS_ROAR, MZ_HUGE), MR_ELEC, 0, */
/*         M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_CARNIVORE, */
/*         M2_HOSTILE | M2_STRONG | M2_GREEDY | M2_JEWELS, 0, CLR_BLUE), */
/*     MON("baby green dragon", S_DRAGON, LVL(12, 9, 2, 10, 0), G_GENO, */
/*         A(ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(1500, 500, MS_ROAR, MZ_HUGE), MR_POISON, 0, */
/*         M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_CARNIVORE | M1_POIS, */
/*         M2_HOSTILE | M2_STRONG | M2_GREEDY | M2_JEWELS, 0, CLR_GREEN), */
/*     MON("baby yellow dragon", S_DRAGON, LVL(12, 9, 2, 10, 0), G_GENO, */
/*         A(ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(1500, 500, MS_ROAR, MZ_HUGE), MR_ACID | MR_STONE, 0, */
/*         M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_CARNIVORE | M1_ACID, */
/*         M2_HOSTILE | M2_STRONG | M2_GREEDY | M2_JEWELS, 0, CLR_YELLOW), */
/*     MON("gray dragon", S_DRAGON, LVL(15, 9, -1, 20, 4), (G_GENO | 1), */
/*         A(ATTK(AT_BREA, AD_MAGM, 4, 6), ATTK(AT_BITE, AD_PHYS, 3, 8), */
/*           ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4), NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_DRAGON, 1500, MS_ROAR, MZ_GIGANTIC), 0, 0, */
/*         M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_SEE_INVIS | M1_OVIPAROUS */
/*             | M1_CARNIVORE, */
/*         M2_HOSTILE | M2_STRONG | M2_NASTY | M2_GREEDY | M2_JEWELS | M2_MAGIC, */
/*         0, CLR_GRAY), */
/*     MON("silver dragon", S_DRAGON, LVL(15, 9, -1, 20, 4), (G_GENO | 1), */
/*         A(ATTK(AT_BREA, AD_COLD, 4, 6), ATTK(AT_BITE, AD_PHYS, 3, 8), */
/*           ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4), NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_DRAGON, 1500, MS_ROAR, MZ_GIGANTIC), MR_COLD, 0, */
/*         M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_SEE_INVIS | M1_OVIPAROUS */
/*             | M1_CARNIVORE, */
/*         M2_HOSTILE | M2_STRONG | M2_NASTY | M2_GREEDY | M2_JEWELS | M2_MAGIC, */
/*         0, DRAGON_SILVER), */
/* #if 0 #<{(| DEFERRED |)}># */
/*     MON("shimmering dragon", S_DRAGON, */
/*         LVL(15, 9, -1, 20, 4), (G_GENO | 1), */
/*         A(ATTK(AT_BREA, AD_MAGM, 4, 6), ATTK(AT_BITE, AD_PHYS, 3, 8), */
/*           ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4), */
/*           NO_ATTK, NO_ATTK), */
/*         SIZ(WT_DRAGON, 1500, MS_ROAR, MZ_GIGANTIC), 0, 0, */
/*         M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_SEE_INVIS | M1_OVIPAROUS */
/*           | M1_CARNIVORE, */
/*         M2_HOSTILE | M2_STRONG | M2_NASTY | M2_GREEDY | M2_JEWELS | M2_MAGIC, */
/*         0, CLR_CYAN), */
/* #endif */
/*     MON("red dragon", S_DRAGON, LVL(15, 9, -1, 20, -4), (G_GENO | 1), */
/*         A(ATTK(AT_BREA, AD_FIRE, 6, 6), ATTK(AT_BITE, AD_PHYS, 3, 8), */
/*           ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4), NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_DRAGON, 1500, MS_ROAR, MZ_GIGANTIC), MR_FIRE, MR_FIRE, */
/*         M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_SEE_INVIS | M1_OVIPAROUS */
/*             | M1_CARNIVORE, */
/*         M2_HOSTILE | M2_STRONG | M2_NASTY | M2_GREEDY | M2_JEWELS | M2_MAGIC, */
/*         M3_INFRAVISIBLE, CLR_RED), */
/*     MON("white dragon", S_DRAGON, LVL(15, 9, -1, 20, -5), (G_GENO | 1), */
/*         A(ATTK(AT_BREA, AD_COLD, 4, 6), ATTK(AT_BITE, AD_PHYS, 3, 8), */
/*           ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4), NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_DRAGON, 1500, MS_ROAR, MZ_GIGANTIC), MR_COLD, MR_COLD, */
/*         M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_SEE_INVIS | M1_OVIPAROUS */
/*             | M1_CARNIVORE, */
/*         M2_HOSTILE | M2_STRONG | M2_NASTY | M2_GREEDY | M2_JEWELS | M2_MAGIC, */
/*         0, CLR_WHITE), */
/*     MON("orange dragon", S_DRAGON, LVL(15, 9, -1, 20, 5), (G_GENO | 1), */
/*         A(ATTK(AT_BREA, AD_SLEE, 4, 25), ATTK(AT_BITE, AD_PHYS, 3, 8), */
/*           ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4), NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_DRAGON, 1500, MS_ROAR, MZ_GIGANTIC), MR_SLEEP, MR_SLEEP, */
/*         M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_SEE_INVIS | M1_OVIPAROUS */
/*             | M1_CARNIVORE, */
/*         M2_HOSTILE | M2_STRONG | M2_NASTY | M2_GREEDY | M2_JEWELS | M2_MAGIC, */
/*         0, CLR_ORANGE), */
/*     #<{(| disintegration breath is actually all or nothing, not 1d255 |)}># */
/*     MON("black dragon", S_DRAGON, LVL(15, 9, -1, 20, -6), (G_GENO | 1), */
/*         A(ATTK(AT_BREA, AD_DISN, 1, 255), ATTK(AT_BITE, AD_PHYS, 3, 8), */
/*           ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4), NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_DRAGON, 1500, MS_ROAR, MZ_GIGANTIC), MR_DISINT, MR_DISINT, */
/*         M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_SEE_INVIS | M1_OVIPAROUS */
/*             | M1_CARNIVORE, */
/*         M2_HOSTILE | M2_STRONG | M2_NASTY | M2_GREEDY | M2_JEWELS | M2_MAGIC, */
/*         0, CLR_BLACK), */
/*     MON("blue dragon", S_DRAGON, LVL(15, 9, -1, 20, -7), (G_GENO | 1), */
/*         A(ATTK(AT_BREA, AD_ELEC, 4, 6), ATTK(AT_BITE, AD_PHYS, 3, 8), */
/*           ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4), NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_DRAGON, 1500, MS_ROAR, MZ_GIGANTIC), MR_ELEC, MR_ELEC, */
/*         M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_SEE_INVIS | M1_OVIPAROUS */
/*             | M1_CARNIVORE, */
/*         M2_HOSTILE | M2_STRONG | M2_NASTY | M2_GREEDY | M2_JEWELS | M2_MAGIC, */
/*         0, CLR_BLUE), */
/*     MON("green dragon", S_DRAGON, LVL(15, 9, -1, 20, 6), (G_GENO | 1), */
/*         A(ATTK(AT_BREA, AD_DRST, 4, 6), ATTK(AT_BITE, AD_PHYS, 3, 8), */
/*           ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4), NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_DRAGON, 1500, MS_ROAR, MZ_GIGANTIC), MR_POISON, MR_POISON, */
/*         M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_SEE_INVIS | M1_OVIPAROUS */
/*             | M1_CARNIVORE | M1_POIS, */
/*         M2_HOSTILE | M2_STRONG | M2_NASTY | M2_GREEDY | M2_JEWELS | M2_MAGIC, */
/*         0, CLR_GREEN), */
/*     MON("yellow dragon", S_DRAGON, LVL(15, 9, -1, 20, 7), (G_GENO | 1), */
/*         A(ATTK(AT_BREA, AD_ACID, 4, 6), ATTK(AT_BITE, AD_PHYS, 3, 8), */
/*           ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4), NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_DRAGON, 1500, MS_ROAR, MZ_GIGANTIC), MR_ACID | MR_STONE, */
/*         MR_STONE, M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_SEE_INVIS */
/*                       | M1_OVIPAROUS | M1_CARNIVORE | M1_ACID, */
/*         M2_HOSTILE | M2_STRONG | M2_NASTY | M2_GREEDY | M2_JEWELS | M2_MAGIC, */
/*         0, CLR_YELLOW), */
/*     #<{(| */
/*      * Elementals */
/*      |)}># */
/*     MON("stalker", S_ELEMENTAL, LVL(8, 12, 3, 0, 0), (G_GENO | 3), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 4, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(900, 400, MS_SILENT, MZ_LARGE), 0, 0, */
/*         M1_ANIMAL | M1_FLY | M1_SEE_INVIS, */
/*         M2_WANDER | M2_STALK | M2_HOSTILE | M2_STRONG, M3_INFRAVISION, */
/*         CLR_WHITE), */
/*     MON("air elemental", S_ELEMENTAL, LVL(8, 36, 2, 30, 0), (G_NOCORPSE | 1), */
/*         A(ATTK(AT_ENGL, AD_PHYS, 1, 10), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(0, 0, MS_SILENT, MZ_HUGE), MR_POISON | MR_STONE, 0, */
/*         M1_NOEYES | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS | M1_BREATHLESS */
/*             | M1_UNSOLID | M1_FLY, */
/*         M2_STRONG | M2_NEUTER, 0, CLR_CYAN), */
/*     MON("fire elemental", S_ELEMENTAL, LVL(8, 12, 2, 30, 0), (G_NOCORPSE | 1), */
/*         A(ATTK(AT_CLAW, AD_FIRE, 3, 6), ATTK(AT_NONE, AD_FIRE, 0, 4), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(0, 0, MS_SILENT, MZ_HUGE), MR_FIRE | MR_POISON | MR_STONE, 0, */
/*         M1_NOEYES | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS | M1_BREATHLESS */
/*             | M1_UNSOLID | M1_FLY | M1_NOTAKE, */
/*         M2_STRONG | M2_NEUTER, M3_INFRAVISIBLE, CLR_YELLOW), */
/*     MON("earth elemental", S_ELEMENTAL, LVL(8, 6, 2, 30, 0), (G_NOCORPSE | 1), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 4, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(2500, 0, MS_SILENT, MZ_HUGE), */
/*         MR_FIRE | MR_COLD | MR_POISON | MR_STONE, 0, */
/*         M1_NOEYES | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS | M1_BREATHLESS */
/*             | M1_WALLWALK | M1_THICK_HIDE, */
/*         M2_STRONG | M2_NEUTER, 0, CLR_BROWN), */
/*     MON("water elemental", S_ELEMENTAL, LVL(8, 6, 2, 30, 0), (G_NOCORPSE | 1), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 5, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(2500, 0, MS_SILENT, MZ_HUGE), MR_POISON | MR_STONE, 0, */
/*         M1_NOEYES | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS | M1_BREATHLESS */
/*             | M1_UNSOLID | M1_AMPHIBIOUS | M1_SWIM, */
/*         M2_STRONG | M2_NEUTER, 0, CLR_BLUE), */
/*     #<{(| */
/*      * Fungi */
/*      |)}># */
/*     MON("lichen", S_FUNGUS, LVL(0, 1, 9, 0, 0), (G_GENO | 4), */
/*         A(ATTK(AT_TUCH, AD_STCK, 0, 0), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(20, 200, MS_SILENT, MZ_SMALL), 0, 0, */
/*         M1_BREATHLESS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS */
/*             | M1_NOTAKE, */
/*         M2_HOSTILE | M2_NEUTER, 0, CLR_BRIGHT_GREEN), */
/*     MON("brown mold", S_FUNGUS, LVL(1, 0, 9, 0, 0), (G_GENO | 1), */
/*         A(ATTK(AT_NONE, AD_COLD, 0, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(50, 30, MS_SILENT, MZ_SMALL), MR_COLD | MR_POISON, */
/*         MR_COLD | MR_POISON, M1_BREATHLESS | M1_NOEYES | M1_NOLIMBS */
/*                                  | M1_NOHEAD | M1_MINDLESS | M1_NOTAKE, */
/*         M2_HOSTILE | M2_NEUTER, 0, CLR_BROWN), */
/*     MON("yellow mold", S_FUNGUS, LVL(1, 0, 9, 0, 0), (G_GENO | 2), */
/*         A(ATTK(AT_NONE, AD_STUN, 0, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(50, 30, MS_SILENT, MZ_SMALL), MR_POISON, MR_POISON, */
/*         M1_BREATHLESS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS */
/*             | M1_POIS | M1_NOTAKE, */
/*         M2_HOSTILE | M2_NEUTER, 0, CLR_YELLOW), */
/*     MON("green mold", S_FUNGUS, LVL(1, 0, 9, 0, 0), (G_GENO | 1), */
/*         A(ATTK(AT_NONE, AD_ACID, 0, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(50, 30, MS_SILENT, MZ_SMALL), MR_ACID | MR_STONE, MR_STONE, */
/*         M1_BREATHLESS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS */
/*             | M1_ACID | M1_NOTAKE, */
/*         M2_HOSTILE | M2_NEUTER, 0, CLR_GREEN), */
/*     MON("red mold", S_FUNGUS, LVL(1, 0, 9, 0, 0), (G_GENO | 1), */
/*         A(ATTK(AT_NONE, AD_FIRE, 0, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(50, 30, MS_SILENT, MZ_SMALL), MR_FIRE | MR_POISON, */
/*         MR_FIRE | MR_POISON, M1_BREATHLESS | M1_NOEYES | M1_NOLIMBS */
/*                                  | M1_NOHEAD | M1_MINDLESS | M1_NOTAKE, */
/*         M2_HOSTILE | M2_NEUTER, M3_INFRAVISIBLE, CLR_RED), */
/*     MON("shrieker", S_FUNGUS, LVL(3, 1, 7, 0, 0), (G_GENO | 1), */
/*         A(NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(100, 100, MS_SHRIEK, MZ_SMALL), MR_POISON, MR_POISON, */
/*         M1_BREATHLESS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS */
/*             | M1_NOTAKE, */
/*         M2_HOSTILE | M2_NEUTER, 0, CLR_MAGENTA), */
/*     MON("violet fungus", S_FUNGUS, LVL(3, 1, 7, 0, 0), (G_GENO | 2), */
/*         A(ATTK(AT_TUCH, AD_PHYS, 1, 4), ATTK(AT_TUCH, AD_STCK, 0, 0), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(100, 100, MS_SILENT, MZ_SMALL), MR_POISON, MR_POISON, */
/*         M1_BREATHLESS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS */
/*             | M1_NOTAKE, */
/*         M2_HOSTILE | M2_NEUTER, 0, CLR_MAGENTA), */
/*     #<{(| */
/*      * Gnomes */
/*      |)}># */
/*     MON("gnome", S_GNOME, LVL(1, 6, 10, 4, 0), (G_GENO | G_SGROUP | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(650, 100, MS_ORC, MZ_SMALL), 0, 0, M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_GNOME | M2_COLLECT, M3_INFRAVISIBLE | M3_INFRAVISION, */
/*         CLR_BROWN), */
/*     MON("gnome lord", S_GNOME, LVL(3, 8, 10, 4, 0), (G_GENO | 2), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(700, 120, MS_ORC, MZ_SMALL), 0, 0, M1_HUMANOID | M1_OMNIVORE, */
/*         M2_GNOME | M2_LORD | M2_MALE | M2_COLLECT, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BLUE), */
/*     MON("gnomish wizard", S_GNOME, LVL(3, 10, 4, 10, 0), (G_GENO | 1), */
/*         A(ATTK(AT_MAGC, AD_SPEL, 0, 0), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(700, 120, MS_ORC, MZ_SMALL), 0, 0, M1_HUMANOID | M1_OMNIVORE, */
/*         M2_GNOME | M2_MAGIC, M3_INFRAVISIBLE | M3_INFRAVISION, HI_ZAP), */
/*     MON("gnome king", S_GNOME, LVL(5, 10, 10, 20, 0), (G_GENO | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(750, 150, MS_ORC, MZ_SMALL), 0, 0, M1_HUMANOID | M1_OMNIVORE, */
/*         M2_GNOME | M2_PRINCE | M2_MALE | M2_COLLECT, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, HI_LORD), */
/* #ifdef SPLITMON_1 */
/* }; */
/* #endif */
/* #endif #<{(| !SPLITMON_2 |)}># */
/*  */
/* #<{(| horrible kludge alert: */
/*  * This is a compiler-specific kludge to allow the compilation of monst.o in */
/*  * two pieces, by defining first SPLITMON_1 and then SPLITMON_2. The */
/*  * resulting assembler files (monst1.s and monst2.s) are then run through */
/*  * sed to change local symbols, concatenated together, and assembled to */
/*  * produce monst.o. THIS ONLY WORKS WITH THE ATARI GCC, and should only */
/*  * be done if you don't have enough memory to compile monst.o the "normal" */
/*  * way.  --ERS */
/*  |)}># */
/*  */
/* #ifndef SPLITMON_1 */
/* #ifdef SPLITMON_2 */
/* struct permonst _mons2[] = { */
/* #endif */
/*     #<{(| */
/*      * giant Humanoids */
/*      |)}># */
/*     MON("giant", S_GIANT, LVL(6, 6, 0, 0, 2), (G_GENO | G_NOGEN | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 10), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(2250, 750, MS_BOAST, MZ_HUGE), 0, 0, M1_HUMANOID | M1_CARNIVORE, */
/*         M2_GIANT | M2_STRONG | M2_ROCKTHROW | M2_NASTY | M2_COLLECT */
/*             | M2_JEWELS, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED), */
/*     MON("stone giant", S_GIANT, LVL(6, 6, 0, 0, 2), (G_GENO | G_SGROUP | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 10), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(2250, 750, MS_BOAST, MZ_HUGE), 0, 0, M1_HUMANOID | M1_CARNIVORE, */
/*         M2_GIANT | M2_STRONG | M2_ROCKTHROW | M2_NASTY | M2_COLLECT */
/*             | M2_JEWELS, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_GRAY), */
/*     MON("hill giant", S_GIANT, LVL(8, 10, 6, 0, -2), (G_GENO | G_SGROUP | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(2200, 700, MS_BOAST, MZ_HUGE), 0, 0, M1_HUMANOID | M1_CARNIVORE, */
/*         M2_GIANT | M2_STRONG | M2_ROCKTHROW | M2_NASTY | M2_COLLECT */
/*             | M2_JEWELS, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_CYAN), */
/*     MON("fire giant", S_GIANT, LVL(9, 12, 4, 5, 2), (G_GENO | G_SGROUP | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 10), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(2250, 750, MS_BOAST, MZ_HUGE), MR_FIRE, MR_FIRE, */
/*         M1_HUMANOID | M1_CARNIVORE, M2_GIANT | M2_STRONG | M2_ROCKTHROW */
/*                                         | M2_NASTY | M2_COLLECT | M2_JEWELS, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_YELLOW), */
/*     MON("frost giant", S_GIANT, LVL(10, 12, 3, 10, -3), */
/*         (G_NOHELL | G_GENO | G_SGROUP | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 12), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(2250, 750, MS_BOAST, MZ_HUGE), MR_COLD, MR_COLD, */
/*         M1_HUMANOID | M1_CARNIVORE, M2_GIANT | M2_STRONG | M2_ROCKTHROW */
/*                                         | M2_NASTY | M2_COLLECT | M2_JEWELS, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_WHITE), */
/*     MON("ettin", S_GIANT, LVL(10, 12, 3, 0, 0), (G_GENO | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 8), ATTK(AT_WEAP, AD_PHYS, 3, 6), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1700, 500, MS_GRUNT, MZ_HUGE), 0, 0, */
/*         M1_ANIMAL | M1_HUMANOID | M1_CARNIVORE, */
/*         M2_HOSTILE | M2_STRONG | M2_NASTY | M2_COLLECT, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BROWN), */
/*     MON("storm giant", S_GIANT, LVL(16, 12, 3, 10, -3), */
/*         (G_GENO | G_SGROUP | 1), A(ATTK(AT_WEAP, AD_PHYS, 2, 12), NO_ATTK, */
/*                                    NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(2250, 750, MS_BOAST, MZ_HUGE), MR_ELEC, MR_ELEC, */
/*         M1_HUMANOID | M1_CARNIVORE, M2_GIANT | M2_STRONG | M2_ROCKTHROW */
/*                                         | M2_NASTY | M2_COLLECT | M2_JEWELS, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BLUE), */
/*     MON("titan", S_GIANT, LVL(16, 18, -3, 70, 9), (1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 8), ATTK(AT_MAGC, AD_SPEL, 0, 0), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(2300, 900, MS_SPELL, MZ_HUGE), 0, 0, */
/*         M1_FLY | M1_HUMANOID | M1_OMNIVORE, */
/*         M2_STRONG | M2_ROCKTHROW | M2_NASTY | M2_COLLECT | M2_MAGIC, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_MAGENTA), */
/*     MON("minotaur", S_GIANT, LVL(15, 15, 6, 0, 0), (G_GENO | G_NOGEN), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 3, 10), ATTK(AT_CLAW, AD_PHYS, 3, 10), */
/*           ATTK(AT_BUTT, AD_PHYS, 2, 8), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1500, 700, MS_SILENT, MZ_LARGE), 0, 0, */
/*         M1_ANIMAL | M1_HUMANOID | M1_CARNIVORE, */
/*         M2_HOSTILE | M2_STRONG | M2_NASTY, M3_INFRAVISIBLE | M3_INFRAVISION, */
/*         CLR_BROWN), */
/*     #<{(| 'I' is a visual marker for all invisible monsters and must be unused |)}># */
/*     #<{(| */
/*      * Jabberwock */
/*      |)}># */
/*     #<{(| the illustration from _Through_the_Looking_Glass_ */
/*        depicts hands as well as wings |)}># */
/*     MON("jabberwock", S_JABBERWOCK, LVL(15, 12, -2, 50, 0), (G_GENO | 1), */
/*         A(ATTK(AT_BITE, AD_PHYS, 2, 10), ATTK(AT_BITE, AD_PHYS, 2, 10), */
/*           ATTK(AT_CLAW, AD_PHYS, 2, 10), ATTK(AT_CLAW, AD_PHYS, 2, 10), */
/*           NO_ATTK, NO_ATTK), */
/*         SIZ(1300, 600, MS_BURBLE, MZ_LARGE), 0, 0, */
/*         M1_ANIMAL | M1_FLY | M1_CARNIVORE, */
/*         M2_HOSTILE | M2_STRONG | M2_NASTY | M2_COLLECT, M3_INFRAVISIBLE, */
/*         CLR_ORANGE), */
/* #if 0 #<{(| DEFERRED |)}># */
/*     MON("vorpal jabberwock", S_JABBERWOCK, */
/*         LVL(20, 12, -2, 50, 0), (G_GENO | 1), */
/*         A(ATTK(AT_BITE, AD_PHYS, 3, 10), ATTK(AT_BITE, AD_PHYS, 3, 10), */
/*           ATTK(AT_CLAW, AD_PHYS, 3, 10), ATTK(AT_CLAW, AD_PHYS, 3, 10), */
/*           NO_ATTK, NO_ATTK), */
/*         SIZ(1300, 600, MS_BURBLE, MZ_LARGE), 0, 0, */
/*         M1_ANIMAL | M1_FLY | M1_CARNIVORE, */
/*         M2_HOSTILE | M2_STRONG | M2_NASTY | M2_COLLECT, M3_INFRAVISIBLE, */
/*         HI_LORD), */
/* #endif */
/*     #<{(| */
/*      * Kops */
/*      |)}># */
/*     MON("Keystone Kop", S_KOP, LVL(1, 6, 10, 10, 9), */
/*         (G_GENO | G_LGROUP | G_NOGEN), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 200, MS_ARREST, MZ_HUMAN), 0, 0, M1_HUMANOID, */
/*         M2_HUMAN | M2_WANDER | M2_HOSTILE | M2_MALE | M2_COLLECT, */
/*         M3_INFRAVISIBLE, CLR_BLUE), */
/*     MON("Kop Sergeant", S_KOP, LVL(2, 8, 10, 10, 10), */
/*         (G_GENO | G_SGROUP | G_NOGEN), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 200, MS_ARREST, MZ_HUMAN), 0, 0, M1_HUMANOID, */
/*         M2_HUMAN | M2_WANDER | M2_HOSTILE | M2_STRONG | M2_MALE | M2_COLLECT, */
/*         M3_INFRAVISIBLE, CLR_BLUE), */
/*     MON("Kop Lieutenant", S_KOP, LVL(3, 10, 10, 20, 11), (G_GENO | G_NOGEN), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 200, MS_ARREST, MZ_HUMAN), 0, 0, M1_HUMANOID, */
/*         M2_HUMAN | M2_WANDER | M2_HOSTILE | M2_STRONG | M2_MALE | M2_COLLECT, */
/*         M3_INFRAVISIBLE, CLR_CYAN), */
/*     MON("Kop Kaptain", S_KOP, LVL(4, 12, 10, 20, 12), (G_GENO | G_NOGEN), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 200, MS_ARREST, MZ_HUMAN), 0, 0, M1_HUMANOID, */
/*         M2_HUMAN | M2_WANDER | M2_HOSTILE | M2_STRONG | M2_MALE | M2_COLLECT, */
/*         M3_INFRAVISIBLE, HI_LORD), */
/*     #<{(| */
/*      * Liches */
/*      |)}># */
/*     MON("lich", S_LICH, LVL(11, 6, 0, 30, -9), (G_GENO | G_NOCORPSE | 1), */
/*         A(ATTK(AT_TUCH, AD_COLD, 1, 10), ATTK(AT_MAGC, AD_SPEL, 0, 0), */
/*           NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1200, 100, MS_MUMBLE, MZ_HUMAN), MR_COLD | MR_SLEEP | MR_POISON, */
/*         MR_COLD, M1_BREATHLESS | M1_HUMANOID | M1_POIS | M1_REGEN, */
/*         M2_UNDEAD | M2_HOSTILE | M2_MAGIC, M3_INFRAVISION, CLR_BROWN), */
/*     MON("demilich", S_LICH, LVL(14, 9, -2, 60, -12), */
/*         (G_GENO | G_NOCORPSE | 1), */
/*         A(ATTK(AT_TUCH, AD_COLD, 3, 4), ATTK(AT_MAGC, AD_SPEL, 0, 0), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1200, 100, MS_MUMBLE, MZ_HUMAN), MR_COLD | MR_SLEEP | MR_POISON, */
/*         MR_COLD, M1_BREATHLESS | M1_HUMANOID | M1_POIS | M1_REGEN, */
/*         M2_UNDEAD | M2_HOSTILE | M2_MAGIC, M3_INFRAVISION, CLR_RED), */
/*     MON("master lich", S_LICH, LVL(17, 9, -4, 90, -15), */
/*         (G_HELL | G_GENO | G_NOCORPSE | 1), */
/*         A(ATTK(AT_TUCH, AD_COLD, 3, 6), ATTK(AT_MAGC, AD_SPEL, 0, 0), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1200, 100, MS_MUMBLE, MZ_HUMAN), */
/*         MR_FIRE | MR_COLD | MR_SLEEP | MR_POISON, MR_FIRE | MR_COLD, */
/*         M1_BREATHLESS | M1_HUMANOID | M1_POIS | M1_REGEN, */
/*         M2_UNDEAD | M2_HOSTILE | M2_MAGIC, M3_WANTSBOOK | M3_INFRAVISION, */
/*         HI_LORD), */
/*     MON("arch-lich", S_LICH, LVL(25, 9, -6, 90, -15), */
/*         (G_HELL | G_GENO | G_NOCORPSE | 1), */
/*         A(ATTK(AT_TUCH, AD_COLD, 5, 6), ATTK(AT_MAGC, AD_SPEL, 0, 0), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1200, 100, MS_MUMBLE, MZ_HUMAN), */
/*         MR_FIRE | MR_COLD | MR_SLEEP | MR_ELEC | MR_POISON, MR_FIRE | MR_COLD, */
/*         M1_BREATHLESS | M1_HUMANOID | M1_POIS | M1_REGEN, */
/*         M2_UNDEAD | M2_HOSTILE | M2_MAGIC, M3_WANTSBOOK | M3_INFRAVISION, */
/*         HI_LORD), */
/*     #<{(| */
/*      * Mummies */
/*      |)}># */
/*     MON("kobold mummy", S_MUMMY, LVL(3, 8, 6, 20, -2), */
/*         (G_GENO | G_NOCORPSE | 1), A(ATTK(AT_CLAW, AD_PHYS, 1, 4), NO_ATTK, */
/*                                      NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(400, 50, MS_SILENT, MZ_SMALL), MR_COLD | MR_SLEEP | MR_POISON, 0, */
/*         M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_POIS, */
/*         M2_UNDEAD | M2_HOSTILE, M3_INFRAVISION, CLR_BROWN), */
/*     MON("gnome mummy", S_MUMMY, LVL(4, 10, 6, 20, -3), */
/*         (G_GENO | G_NOCORPSE | 1), A(ATTK(AT_CLAW, AD_PHYS, 1, 6), NO_ATTK, */
/*                                      NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(650, 50, MS_SILENT, MZ_SMALL), MR_COLD | MR_SLEEP | MR_POISON, 0, */
/*         M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_POIS, */
/*         M2_UNDEAD | M2_HOSTILE | M2_GNOME, M3_INFRAVISION, CLR_RED), */
/*     MON("orc mummy", S_MUMMY, LVL(5, 10, 5, 20, -4), */
/*         (G_GENO | G_NOCORPSE | 1), A(ATTK(AT_CLAW, AD_PHYS, 1, 6), NO_ATTK, */
/*                                      NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(850, 75, MS_SILENT, MZ_HUMAN), MR_COLD | MR_SLEEP | MR_POISON, 0, */
/*         M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_POIS, */
/*         M2_UNDEAD | M2_HOSTILE | M2_ORC | M2_GREEDY | M2_JEWELS, */
/*         M3_INFRAVISION, CLR_GRAY), */
/*     MON("dwarf mummy", S_MUMMY, LVL(5, 10, 5, 20, -4), */
/*         (G_GENO | G_NOCORPSE | 1), A(ATTK(AT_CLAW, AD_PHYS, 1, 6), NO_ATTK, */
/*                                      NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(900, 150, MS_SILENT, MZ_HUMAN), MR_COLD | MR_SLEEP | MR_POISON, 0, */
/*         M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_POIS, */
/*         M2_UNDEAD | M2_HOSTILE | M2_DWARF | M2_GREEDY | M2_JEWELS, */
/*         M3_INFRAVISION, CLR_RED), */
/*     MON("elf mummy", S_MUMMY, LVL(6, 12, 4, 30, -5), */
/*         (G_GENO | G_NOCORPSE | 1), A(ATTK(AT_CLAW, AD_PHYS, 2, 4), NO_ATTK, */
/*                                      NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_ELF, 175, MS_SILENT, MZ_HUMAN), MR_COLD | MR_SLEEP | MR_POISON, */
/*         0, M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_POIS, */
/*         M2_UNDEAD | M2_HOSTILE | M2_ELF, M3_INFRAVISION, CLR_GREEN), */
/*     MON("human mummy", S_MUMMY, LVL(6, 12, 4, 30, -5), */
/*         (G_GENO | G_NOCORPSE | 1), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 2, 4), ATTK(AT_CLAW, AD_PHYS, 2, 4), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 200, MS_SILENT, MZ_HUMAN), */
/*         MR_COLD | MR_SLEEP | MR_POISON, 0, */
/*         M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_POIS, */
/*         M2_UNDEAD | M2_HOSTILE, M3_INFRAVISION, CLR_GRAY), */
/*     MON("ettin mummy", S_MUMMY, LVL(7, 12, 4, 30, -6), */
/*         (G_GENO | G_NOCORPSE | 1), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 2, 6), ATTK(AT_CLAW, AD_PHYS, 2, 6), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1700, 250, MS_SILENT, MZ_HUGE), MR_COLD | MR_SLEEP | MR_POISON, 0, */
/*         M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_POIS, */
/*         M2_UNDEAD | M2_HOSTILE | M2_STRONG, M3_INFRAVISION, CLR_BLUE), */
/*     MON("giant mummy", S_MUMMY, LVL(8, 14, 3, 30, -7), */
/*         (G_GENO | G_NOCORPSE | 1), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 3, 4), ATTK(AT_CLAW, AD_PHYS, 3, 4), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(2050, 375, MS_SILENT, MZ_HUGE), MR_COLD | MR_SLEEP | MR_POISON, 0, */
/*         M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_POIS, */
/*         M2_UNDEAD | M2_HOSTILE | M2_GIANT | M2_STRONG | M2_JEWELS, */
/*         M3_INFRAVISION, CLR_CYAN), */
/*     #<{(| */
/*      * Nagas */
/*      |)}># */
/*     MON("red naga hatchling", S_NAGA, LVL(3, 10, 6, 0, 0), G_GENO, */
/*         A(ATTK(AT_BITE, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(500, 100, MS_MUMBLE, MZ_LARGE), MR_FIRE | MR_POISON, */
/*         MR_FIRE | MR_POISON, */
/*         M1_NOLIMBS | M1_SLITHY | M1_THICK_HIDE | M1_NOTAKE | M1_OMNIVORE, */
/*         M2_STRONG, M3_INFRAVISIBLE, CLR_RED), */
/*     MON("black naga hatchling", S_NAGA, LVL(3, 10, 6, 0, 0), G_GENO, */
/*         A(ATTK(AT_BITE, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(500, 100, MS_MUMBLE, MZ_LARGE), MR_POISON | MR_ACID | MR_STONE, */
/*         MR_POISON | MR_STONE, M1_NOLIMBS | M1_SLITHY | M1_THICK_HIDE | M1_ACID */
/*                                   | M1_NOTAKE | M1_CARNIVORE, */
/*         M2_STRONG, 0, CLR_BLACK), */
/*     MON("golden naga hatchling", S_NAGA, LVL(3, 10, 6, 0, 0), G_GENO, */
/*         A(ATTK(AT_BITE, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(500, 100, MS_MUMBLE, MZ_LARGE), MR_POISON, MR_POISON, */
/*         M1_NOLIMBS | M1_SLITHY | M1_THICK_HIDE | M1_NOTAKE | M1_OMNIVORE, */
/*         M2_STRONG, 0, HI_GOLD), */
/*     MON("guardian naga hatchling", S_NAGA, LVL(3, 10, 6, 0, 0), G_GENO, */
/*         A(ATTK(AT_BITE, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(500, 100, MS_MUMBLE, MZ_LARGE), MR_POISON, MR_POISON, */
/*         M1_NOLIMBS | M1_SLITHY | M1_THICK_HIDE | M1_NOTAKE | M1_OMNIVORE, */
/*         M2_STRONG, 0, CLR_GREEN), */
/*     MON("red naga", S_NAGA, LVL(6, 12, 4, 0, -4), (G_GENO | 1), */
/*         A(ATTK(AT_BITE, AD_PHYS, 2, 4), ATTK(AT_BREA, AD_FIRE, 2, 6), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(2600, 400, MS_MUMBLE, MZ_HUGE), MR_FIRE | MR_POISON, */
/*         MR_FIRE | MR_POISON, M1_NOLIMBS | M1_SLITHY | M1_THICK_HIDE */
/*                                  | M1_OVIPAROUS | M1_NOTAKE | M1_OMNIVORE, */
/*         M2_STRONG, M3_INFRAVISIBLE, CLR_RED), */
/*     MON("black naga", S_NAGA, LVL(8, 14, 2, 10, 4), (G_GENO | 1), */
/*         A(ATTK(AT_BITE, AD_PHYS, 2, 6), ATTK(AT_SPIT, AD_ACID, 0, 0), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(2600, 400, MS_MUMBLE, MZ_HUGE), MR_POISON | MR_ACID | MR_STONE, */
/*         MR_POISON | MR_STONE, */
/*         M1_NOLIMBS | M1_SLITHY | M1_THICK_HIDE | M1_OVIPAROUS | M1_ACID */
/*             | M1_NOTAKE | M1_CARNIVORE, */
/*         M2_STRONG, 0, CLR_BLACK), */
/*     MON("golden naga", S_NAGA, LVL(10, 14, 2, 70, 5), (G_GENO | 1), */
/*         A(ATTK(AT_BITE, AD_PHYS, 2, 6), ATTK(AT_MAGC, AD_SPEL, 4, 6), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(2600, 400, MS_MUMBLE, MZ_HUGE), MR_POISON, MR_POISON, */
/*         M1_NOLIMBS | M1_SLITHY | M1_THICK_HIDE | M1_OVIPAROUS | M1_NOTAKE */
/*             | M1_OMNIVORE, */
/*         M2_STRONG, 0, HI_GOLD), */
/*     MON("guardian naga", S_NAGA, LVL(12, 16, 0, 50, 7), (G_GENO | 1), */
/*         A(ATTK(AT_BITE, AD_PLYS, 1, 6), ATTK(AT_SPIT, AD_DRST, 1, 6), */
/*           ATTK(AT_HUGS, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(2600, 400, MS_MUMBLE, MZ_HUGE), MR_POISON, MR_POISON, */
/*         M1_NOLIMBS | M1_SLITHY | M1_THICK_HIDE | M1_OVIPAROUS | M1_POIS */
/*             | M1_NOTAKE | M1_OMNIVORE, */
/*         M2_STRONG, 0, CLR_GREEN), */
/*     #<{(| */
/*      * Ogres */
/*      |)}># */
/*     MON("ogre", S_OGRE, LVL(5, 10, 5, 0, -3), (G_SGROUP | G_GENO | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 5), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(1600, 500, MS_GRUNT, MZ_LARGE), 0, 0, M1_HUMANOID | M1_CARNIVORE, */
/*         M2_STRONG | M2_GREEDY | M2_JEWELS | M2_COLLECT, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BROWN), */
/*     MON("ogre lord", S_OGRE, LVL(7, 12, 3, 30, -5), (G_GENO | 2), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(1700, 700, MS_GRUNT, MZ_LARGE), 0, 0, M1_HUMANOID | M1_CARNIVORE, */
/*         M2_STRONG | M2_LORD | M2_MALE | M2_GREEDY | M2_JEWELS | M2_COLLECT, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED), */
/*     MON("ogre king", S_OGRE, LVL(9, 14, 4, 60, -7), (G_GENO | 2), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 3, 5), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(1700, 750, MS_GRUNT, MZ_LARGE), 0, 0, M1_HUMANOID | M1_CARNIVORE, */
/*         M2_STRONG | M2_PRINCE | M2_MALE | M2_GREEDY | M2_JEWELS | M2_COLLECT, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, HI_LORD), */
/*     #<{(| */
/*      * Puddings */
/*      * */
/*      * must be in the same order as the pudding globs in objects.c */
/*      |)}># */
/*     MON("gray ooze", S_PUDDING, LVL(3, 1, 8, 0, 0), (G_GENO | G_NOCORPSE | 2), */
/*         A(ATTK(AT_BITE, AD_RUST, 2, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(500, 250, MS_SILENT, MZ_MEDIUM), */
/*         MR_FIRE | MR_COLD | MR_POISON | MR_ACID | MR_STONE, */
/*         MR_FIRE | MR_COLD | MR_POISON, */
/*         M1_BREATHLESS | M1_AMORPHOUS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD */
/*             | M1_MINDLESS | M1_OMNIVORE | M1_ACID, */
/*         M2_HOSTILE | M2_NEUTER, 0, CLR_GRAY), */
/*     MON("brown pudding", S_PUDDING, LVL(5, 3, 8, 0, 0), */
/*         (G_GENO | G_NOCORPSE | 1), A(ATTK(AT_BITE, AD_DCAY, 0, 0), NO_ATTK, */
/*                                      NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(500, 250, MS_SILENT, MZ_MEDIUM), */
/*         MR_COLD | MR_ELEC | MR_POISON | MR_ACID | MR_STONE, */
/*         MR_COLD | MR_ELEC | MR_POISON, */
/*         M1_BREATHLESS | M1_AMORPHOUS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD */
/*             | M1_MINDLESS | M1_OMNIVORE | M1_ACID, */
/*         M2_HOSTILE | M2_NEUTER, 0, CLR_BROWN), */
/*     MON("green slime", S_PUDDING, LVL(6, 6, 6, 0, 0), */
/*         (G_HELL | G_GENO | G_NOCORPSE | 1), */
/*         A(ATTK(AT_TUCH, AD_SLIM, 1, 4), ATTK(AT_NONE, AD_SLIM, 0, 0), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(400, 150, MS_SILENT, MZ_LARGE), */
/*         MR_COLD | MR_ELEC | MR_POISON | MR_ACID | MR_STONE, 0, */
/*         M1_BREATHLESS | M1_AMORPHOUS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD */
/*             | M1_MINDLESS | M1_OMNIVORE | M1_ACID | M1_POIS, */
/*         M2_HOSTILE | M2_NEUTER, 0, CLR_GREEN), */
/*     MON("black pudding", S_PUDDING, LVL(10, 6, 6, 0, 0), */
/*         (G_GENO | G_NOCORPSE | 1), */
/*         A(ATTK(AT_BITE, AD_CORR, 3, 8), ATTK(AT_NONE, AD_CORR, 0, 0), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(900, 250, MS_SILENT, MZ_LARGE), */
/*         MR_COLD | MR_ELEC | MR_POISON | MR_ACID | MR_STONE, */
/*         MR_COLD | MR_ELEC | MR_POISON, */
/*         M1_BREATHLESS | M1_AMORPHOUS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD */
/*             | M1_MINDLESS | M1_OMNIVORE | M1_ACID, */
/*         M2_HOSTILE | M2_NEUTER, 0, CLR_BLACK), */
/*     #<{(| */
/*      * Quantum mechanics */
/*      |)}># */
/*     MON("quantum mechanic", S_QUANTMECH, LVL(7, 12, 3, 10, 0), (G_GENO | 3), */
/*         A(ATTK(AT_CLAW, AD_TLPT, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 20, MS_HUMANOID, MZ_HUMAN), MR_POISON, 0, */
/*         M1_HUMANOID | M1_OMNIVORE | M1_POIS | M1_TPORT, M2_HOSTILE, */
/*         M3_INFRAVISIBLE, CLR_CYAN), */
/*     #<{(| */
/*      * Rust monster or disenchanter */
/*      |)}># */
/*     MON("rust monster", S_RUSTMONST, LVL(5, 18, 2, 0, 0), (G_GENO | 2), */
/*         A(ATTK(AT_TUCH, AD_RUST, 0, 0), ATTK(AT_TUCH, AD_RUST, 0, 0), */
/*           ATTK(AT_NONE, AD_RUST, 0, 0), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1000, 250, MS_SILENT, MZ_MEDIUM), 0, 0, */
/*         M1_SWIM | M1_ANIMAL | M1_NOHANDS | M1_METALLIVORE, M2_HOSTILE, */
/*         M3_INFRAVISIBLE, CLR_BROWN), */
/*     MON("disenchanter", S_RUSTMONST, LVL(12, 12, -10, 0, -3), */
/*         (G_HELL | G_GENO | 2), */
/*         A(ATTK(AT_CLAW, AD_ENCH, 4, 4), ATTK(AT_NONE, AD_ENCH, 0, 0), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(750, 200, MS_GROWL, MZ_LARGE), 0, 0, M1_ANIMAL | M1_CARNIVORE, */
/*         M2_HOSTILE, M3_INFRAVISIBLE, CLR_BLUE), */
/*     #<{(| */
/*      * Snakes */
/*      |)}># */
/*     MON("garter snake", S_SNAKE, LVL(1, 8, 8, 0, 0), (G_LGROUP | G_GENO | 1), */
/*         A(ATTK(AT_BITE, AD_PHYS, 1, 2), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(50, 60, MS_HISS, MZ_TINY), 0, 0, */
/*         M1_SWIM | M1_CONCEAL | M1_NOLIMBS | M1_ANIMAL | M1_SLITHY */
/*             | M1_OVIPAROUS | M1_CARNIVORE | M1_NOTAKE, */
/*         0, 0, CLR_GREEN), */
/*     MON("snake", S_SNAKE, LVL(4, 15, 3, 0, 0), (G_GENO | 2), */
/*         A(ATTK(AT_BITE, AD_DRST, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(100, 80, MS_HISS, MZ_SMALL), MR_POISON, MR_POISON, */
/*         M1_SWIM | M1_CONCEAL | M1_NOLIMBS | M1_ANIMAL | M1_SLITHY | M1_POIS */
/*             | M1_OVIPAROUS | M1_CARNIVORE | M1_NOTAKE, */
/*         M2_HOSTILE, 0, CLR_BROWN), */
/*     MON("water moccasin", S_SNAKE, LVL(4, 15, 3, 0, 0), */
/*         (G_GENO | G_NOGEN | G_LGROUP), */
/*         A(ATTK(AT_BITE, AD_DRST, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(150, 80, MS_HISS, MZ_SMALL), MR_POISON, MR_POISON, */
/*         M1_SWIM | M1_CONCEAL | M1_NOLIMBS | M1_ANIMAL | M1_SLITHY | M1_POIS */
/*             | M1_CARNIVORE | M1_OVIPAROUS | M1_NOTAKE, */
/*         M2_HOSTILE, 0, CLR_RED), */
/*     MON("python", S_SNAKE, LVL(6, 3, 5, 0, 0), (G_GENO | 1), */
/*         A(ATTK(AT_BITE, AD_PHYS, 1, 4), ATTK(AT_TUCH, AD_PHYS, 0, 0), */
/*           ATTK(AT_HUGS, AD_WRAP, 1, 4), ATTK(AT_HUGS, AD_PHYS, 2, 4), NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(250, 100, MS_HISS, MZ_LARGE), 0, 0, */
/*         M1_SWIM | M1_NOLIMBS | M1_ANIMAL | M1_SLITHY | M1_CARNIVORE */
/*             | M1_OVIPAROUS | M1_NOTAKE, */
/*         M2_HOSTILE | M2_STRONG, M3_INFRAVISION, CLR_MAGENTA), */
/*     MON("pit viper", S_SNAKE, LVL(6, 15, 2, 0, 0), (G_GENO | 1), */
/*         A(ATTK(AT_BITE, AD_DRST, 1, 4), ATTK(AT_BITE, AD_DRST, 1, 4), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(100, 60, MS_HISS, MZ_MEDIUM), MR_POISON, MR_POISON, */
/*         M1_SWIM | M1_CONCEAL | M1_NOLIMBS | M1_ANIMAL | M1_SLITHY | M1_POIS */
/*             | M1_CARNIVORE | M1_OVIPAROUS | M1_NOTAKE, */
/*         M2_HOSTILE, M3_INFRAVISION, CLR_BLUE), */
/*     MON("cobra", S_SNAKE, LVL(6, 18, 2, 0, 0), (G_GENO | 1), */
/*         A(ATTK(AT_BITE, AD_DRST, 2, 4), ATTK(AT_SPIT, AD_BLND, 0, 0), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(250, 100, MS_HISS, MZ_MEDIUM), MR_POISON, MR_POISON, */
/*         M1_SWIM | M1_CONCEAL | M1_NOLIMBS | M1_ANIMAL | M1_SLITHY | M1_POIS */
/*             | M1_CARNIVORE | M1_OVIPAROUS | M1_NOTAKE, */
/*         M2_HOSTILE, 0, CLR_BLUE), */
/*     #<{(| */
/*      * Trolls */
/*      |)}># */
/*     MON("troll", S_TROLL, LVL(7, 12, 4, 0, -3), (G_GENO | 2), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 4, 2), ATTK(AT_CLAW, AD_PHYS, 4, 2), */
/*           ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(800, 350, MS_GRUNT, MZ_LARGE), 0, 0, */
/*         M1_HUMANOID | M1_REGEN | M1_CARNIVORE, */
/*         M2_STRONG | M2_STALK | M2_HOSTILE, M3_INFRAVISIBLE | M3_INFRAVISION, */
/*         CLR_BROWN), */
/*     MON("ice troll", S_TROLL, LVL(9, 10, 2, 20, -3), (G_NOHELL | G_GENO | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 6), ATTK(AT_CLAW, AD_COLD, 2, 6), */
/*           ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1000, 300, MS_GRUNT, MZ_LARGE), MR_COLD, MR_COLD, */
/*         M1_HUMANOID | M1_REGEN | M1_CARNIVORE, */
/*         M2_STRONG | M2_STALK | M2_HOSTILE, M3_INFRAVISIBLE | M3_INFRAVISION, */
/*         CLR_WHITE), */
/*     MON("rock troll", S_TROLL, LVL(9, 12, 0, 0, -3), (G_GENO | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 3, 6), ATTK(AT_CLAW, AD_PHYS, 2, 8), */
/*           ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1200, 300, MS_GRUNT, MZ_LARGE), 0, 0, */
/*         M1_HUMANOID | M1_REGEN | M1_CARNIVORE, */
/*         M2_STRONG | M2_STALK | M2_HOSTILE | M2_COLLECT, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_CYAN), */
/*     MON("water troll", S_TROLL, LVL(11, 14, 4, 40, -3), (G_NOGEN | G_GENO), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 8), ATTK(AT_CLAW, AD_PHYS, 2, 8), */
/*           ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1200, 350, MS_GRUNT, MZ_LARGE), 0, 0, */
/*         M1_HUMANOID | M1_REGEN | M1_CARNIVORE | M1_SWIM, */
/*         M2_STRONG | M2_STALK | M2_HOSTILE, M3_INFRAVISIBLE | M3_INFRAVISION, */
/*         CLR_BLUE), */
/*     MON("Olog-hai", S_TROLL, LVL(13, 12, -4, 0, -7), (G_GENO | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 3, 6), ATTK(AT_CLAW, AD_PHYS, 2, 8), */
/*           ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1500, 400, MS_GRUNT, MZ_LARGE), 0, 0, */
/*         M1_HUMANOID | M1_REGEN | M1_CARNIVORE, */
/*         M2_STRONG | M2_STALK | M2_HOSTILE | M2_COLLECT, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, HI_LORD), */
/*     #<{(| */
/*      * Umber hulk */
/*      |)}># */
/*     MON("umber hulk", S_UMBER, LVL(9, 6, 2, 25, 0), (G_GENO | 2), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 3, 4), ATTK(AT_CLAW, AD_PHYS, 3, 4), */
/*           ATTK(AT_BITE, AD_PHYS, 2, 5), ATTK(AT_GAZE, AD_CONF, 0, 0), NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(1200, 500, MS_SILENT, MZ_LARGE), 0, 0, M1_TUNNEL | M1_CARNIVORE, */
/*         M2_STRONG, M3_INFRAVISIBLE, CLR_BROWN), */
/*     #<{(| */
/*      * Vampires */
/*      |)}># */
/*     MON("vampire", S_VAMPIRE, LVL(10, 12, 1, 25, -8), */
/*         (G_GENO | G_NOCORPSE | 1), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 1, 6), ATTK(AT_BITE, AD_DRLI, 1, 6), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_VAMPIRE, MZ_HUMAN), MR_SLEEP | MR_POISON, 0, */
/*         M1_FLY | M1_BREATHLESS | M1_HUMANOID | M1_POIS | M1_REGEN, */
/*         M2_UNDEAD | M2_STALK | M2_HOSTILE | M2_STRONG | M2_NASTY */
/*             | M2_SHAPESHIFTER, */
/*         M3_INFRAVISIBLE, CLR_RED), */
/*     MON("vampire lord", S_VAMPIRE, LVL(12, 14, 0, 50, -9), */
/*         (G_GENO | G_NOCORPSE | 1), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 1, 8), ATTK(AT_BITE, AD_DRLI, 1, 8), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_VAMPIRE, MZ_HUMAN), MR_SLEEP | MR_POISON, 0, */
/*         M1_FLY | M1_BREATHLESS | M1_HUMANOID | M1_POIS | M1_REGEN, */
/*         M2_UNDEAD | M2_STALK | M2_HOSTILE | M2_STRONG | M2_NASTY | M2_LORD */
/*             | M2_MALE | M2_SHAPESHIFTER, */
/*         M3_INFRAVISIBLE, CLR_BLUE), */
/* #if 0 #<{(| DEFERRED |)}># */
/*     MON("vampire mage", S_VAMPIRE, */
/*         LVL(20, 14, -4, 50, -9), (G_GENO | G_NOCORPSE | 1), */
/*         A(ATTK(AT_CLAW, AD_DRLI, 2, 8), ATTK(AT_BITE, AD_DRLI, 1, 8), */
/*           ATTK(AT_MAGC, AD_SPEL, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_VAMPIRE, MZ_HUMAN), MR_SLEEP | MR_POISON, 0, */
/*         M1_FLY | M1_BREATHLESS | M1_HUMANOID | M1_POIS | M1_REGEN, */
/*         M2_UNDEAD | M2_STALK | M2_HOSTILE | M2_STRONG | M2_NASTY | M2_LORD */
/*           | M2_MALE | M2_MAGIC | M2_SHAPESHIFTER, */
/*         M3_INFRAVISIBLE, HI_ZAP), */
/* #endif */
/*     MON("Vlad the Impaler", S_VAMPIRE, LVL(28, 26, -6, 80, -10), */
/*         (G_NOGEN | G_NOCORPSE | G_UNIQ), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 10), ATTK(AT_BITE, AD_DRLI, 1, 12), */
/*           NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_VAMPIRE, MZ_HUMAN), MR_SLEEP | MR_POISON, 0, */
/*         M1_FLY | M1_BREATHLESS | M1_HUMANOID | M1_POIS | M1_REGEN, */
/*         M2_NOPOLY | M2_UNDEAD | M2_STALK | M2_HOSTILE | M2_PNAME | M2_STRONG */
/*             | M2_NASTY | M2_PRINCE | M2_MALE | M2_SHAPESHIFTER, */
/*         M3_WAITFORU | M3_WANTSCAND | M3_INFRAVISIBLE, HI_LORD), */
/*     #<{(| */
/*      * Wraiths */
/*      |)}># */
/*     MON("barrow wight", S_WRAITH, LVL(3, 12, 5, 5, -3), */
/*         (G_GENO | G_NOCORPSE | 1), */
/*         A(ATTK(AT_WEAP, AD_DRLI, 0, 0), ATTK(AT_MAGC, AD_SPEL, 0, 0), */
/*           ATTK(AT_CLAW, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1200, 0, MS_SPELL, MZ_HUMAN), MR_COLD | MR_SLEEP | MR_POISON, 0, */
/*         M1_BREATHLESS | M1_HUMANOID, */
/*         M2_UNDEAD | M2_STALK | M2_HOSTILE | M2_COLLECT, 0, CLR_GRAY), */
/*     MON("wraith", S_WRAITH, LVL(6, 12, 4, 15, -6), (G_GENO | 2), */
/*         A(ATTK(AT_TUCH, AD_DRLI, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(0, 0, MS_SILENT, MZ_HUMAN), */
/*         MR_COLD | MR_SLEEP | MR_POISON | MR_STONE, 0, */
/*         M1_BREATHLESS | M1_FLY | M1_HUMANOID | M1_UNSOLID, */
/*         M2_UNDEAD | M2_STALK | M2_HOSTILE, 0, CLR_BLACK), */
/*     MON("Nazgul", S_WRAITH, LVL(13, 12, 0, 25, -17), */
/*         (G_GENO | G_NOCORPSE | 1), */
/*         A(ATTK(AT_WEAP, AD_DRLI, 1, 4), ATTK(AT_BREA, AD_SLEE, 2, 25), */
/*           NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 0, MS_SPELL, MZ_HUMAN), MR_COLD | MR_SLEEP | MR_POISON, */
/*         0, M1_BREATHLESS | M1_HUMANOID, */
/*         M2_NOPOLY | M2_UNDEAD | M2_STALK | M2_STRONG | M2_HOSTILE | M2_MALE */
/*             | M2_COLLECT, */
/*         0, HI_LORD), */
/*     #<{(| */
/*      * Xorn */
/*      |)}># */
/*     MON("xorn", S_XORN, LVL(8, 9, -2, 20, 0), (G_GENO | 1), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 1, 3), ATTK(AT_CLAW, AD_PHYS, 1, 3), */
/*           ATTK(AT_CLAW, AD_PHYS, 1, 3), ATTK(AT_BITE, AD_PHYS, 4, 6), NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(1200, 700, MS_ROAR, MZ_MEDIUM), MR_FIRE | MR_COLD | MR_STONE, */
/*         MR_STONE, */
/*         M1_BREATHLESS | M1_WALLWALK | M1_THICK_HIDE | M1_METALLIVORE, */
/*         M2_HOSTILE | M2_STRONG, 0, CLR_BROWN), */
/*     #<{(| */
/*      * Apelike beasts */
/*      |)}># */
/*     #<{(| tameable via banana; does not grow up into ape... */
/*        not flagged as domestic, so no guilt penalty for eating non-pet one |)}># */
/*     MON("monkey", S_YETI, LVL(2, 12, 6, 0, 0), (G_GENO | 1), */
/*         A(ATTK(AT_CLAW, AD_SITM, 0, 0), ATTK(AT_BITE, AD_PHYS, 1, 3), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(100, 50, MS_GROWL, MZ_SMALL), 0, 0, */
/*         M1_ANIMAL | M1_HUMANOID | M1_OMNIVORE, 0, M3_INFRAVISIBLE, CLR_GRAY), */
/*     MON("ape", S_YETI, LVL(4, 12, 6, 0, 0), (G_GENO | G_SGROUP | 2), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 1, 3), ATTK(AT_CLAW, AD_PHYS, 1, 3), */
/*           ATTK(AT_BITE, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1100, 500, MS_GROWL, MZ_LARGE), 0, 0, */
/*         M1_ANIMAL | M1_HUMANOID | M1_OMNIVORE, M2_STRONG, M3_INFRAVISIBLE, */
/*         CLR_BROWN), */
/*     MON("owlbear", S_YETI, LVL(5, 12, 5, 0, 0), (G_GENO | 3), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 1, 6), ATTK(AT_CLAW, AD_PHYS, 1, 6), */
/*           ATTK(AT_HUGS, AD_PHYS, 2, 8), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1700, 700, MS_ROAR, MZ_LARGE), 0, 0, */
/*         M1_ANIMAL | M1_HUMANOID | M1_CARNIVORE, */
/*         M2_HOSTILE | M2_STRONG | M2_NASTY, M3_INFRAVISIBLE, CLR_BROWN), */
/*     MON("yeti", S_YETI, LVL(5, 15, 6, 0, 0), (G_GENO | 2), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 1, 6), ATTK(AT_CLAW, AD_PHYS, 1, 6), */
/*           ATTK(AT_BITE, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1600, 700, MS_GROWL, MZ_LARGE), MR_COLD, MR_COLD, */
/*         M1_ANIMAL | M1_HUMANOID | M1_CARNIVORE, M2_HOSTILE | M2_STRONG, */
/*         M3_INFRAVISIBLE, CLR_WHITE), */
/*     MON("carnivorous ape", S_YETI, LVL(6, 12, 6, 0, 0), (G_GENO | 1), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4), */
/*           ATTK(AT_HUGS, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1250, 550, MS_GROWL, MZ_LARGE), 0, 0, */
/*         M1_ANIMAL | M1_HUMANOID | M1_CARNIVORE, M2_HOSTILE | M2_STRONG, */
/*         M3_INFRAVISIBLE, CLR_BLACK), */
/*     MON("sasquatch", S_YETI, LVL(7, 15, 6, 0, 2), (G_GENO | 1), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 1, 6), ATTK(AT_CLAW, AD_PHYS, 1, 6), */
/*           ATTK(AT_KICK, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1550, 750, MS_GROWL, MZ_LARGE), 0, 0, */
/*         M1_ANIMAL | M1_HUMANOID | M1_SEE_INVIS | M1_OMNIVORE, M2_STRONG, */
/*         M3_INFRAVISIBLE, CLR_GRAY), */
/*     #<{(| */
/*      * Zombies */
/*      |)}># */
/*     MON("kobold zombie", S_ZOMBIE, LVL(0, 6, 10, 0, -2), */
/*         (G_GENO | G_NOCORPSE | 1), A(ATTK(AT_CLAW, AD_PHYS, 1, 4), NO_ATTK, */
/*                                      NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(400, 50, MS_SILENT, MZ_SMALL), MR_COLD | MR_SLEEP | MR_POISON, 0, */
/*         M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_POIS, */
/*         M2_UNDEAD | M2_STALK | M2_HOSTILE, M3_INFRAVISION, CLR_BROWN), */
/*     MON("gnome zombie", S_ZOMBIE, LVL(1, 6, 10, 0, -2), */
/*         (G_GENO | G_NOCORPSE | 1), A(ATTK(AT_CLAW, AD_PHYS, 1, 5), NO_ATTK, */
/*                                      NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(650, 50, MS_SILENT, MZ_SMALL), MR_COLD | MR_SLEEP | MR_POISON, 0, */
/*         M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_POIS, */
/*         M2_UNDEAD | M2_STALK | M2_HOSTILE | M2_GNOME, M3_INFRAVISION, */
/*         CLR_BROWN), */
/*     MON("orc zombie", S_ZOMBIE, LVL(2, 6, 9, 0, -3), */
/*         (G_GENO | G_SGROUP | G_NOCORPSE | 1), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(850, 75, MS_SILENT, MZ_HUMAN), MR_COLD | MR_SLEEP | MR_POISON, 0, */
/*         M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_POIS, */
/*         M2_UNDEAD | M2_STALK | M2_HOSTILE | M2_ORC, M3_INFRAVISION, CLR_GRAY), */
/*     MON("dwarf zombie", S_ZOMBIE, LVL(2, 6, 9, 0, -3), */
/*         (G_GENO | G_SGROUP | G_NOCORPSE | 1), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(900, 150, MS_SILENT, MZ_HUMAN), MR_COLD | MR_SLEEP | MR_POISON, 0, */
/*         M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_POIS, */
/*         M2_UNDEAD | M2_STALK | M2_HOSTILE | M2_DWARF, M3_INFRAVISION, */
/*         CLR_RED), */
/*     MON("elf zombie", S_ZOMBIE, LVL(3, 6, 9, 0, -3), */
/*         (G_GENO | G_SGROUP | G_NOCORPSE | 1), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 1, 7), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_ELF, 175, MS_SILENT, MZ_HUMAN), MR_COLD | MR_SLEEP | MR_POISON, */
/*         0, M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID, */
/*         M2_UNDEAD | M2_STALK | M2_HOSTILE | M2_ELF, M3_INFRAVISION, */
/*         CLR_GREEN), */
/*     MON("human zombie", S_ZOMBIE, LVL(4, 6, 8, 0, -3), */
/*         (G_GENO | G_SGROUP | G_NOCORPSE | 1), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 200, MS_SILENT, MZ_HUMAN), */
/*         MR_COLD | MR_SLEEP | MR_POISON, 0, */
/*         M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID, */
/*         M2_UNDEAD | M2_STALK | M2_HOSTILE, M3_INFRAVISION, HI_DOMESTIC), */
/*     MON("ettin zombie", S_ZOMBIE, LVL(6, 8, 6, 0, -4), */
/*         (G_GENO | G_NOCORPSE | 1), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 1, 10), ATTK(AT_CLAW, AD_PHYS, 1, 10), */
/*           NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1700, 250, MS_SILENT, MZ_HUGE), MR_COLD | MR_SLEEP | MR_POISON, 0, */
/*         M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID, */
/*         M2_UNDEAD | M2_STALK | M2_HOSTILE | M2_STRONG, M3_INFRAVISION, */
/*         CLR_BLUE), */
/*     MON("ghoul", S_ZOMBIE, LVL(3, 6, 10, 0, -2), (G_GENO | G_NOCORPSE | 1), */
/*         A(ATTK(AT_CLAW, AD_PLYS, 1, 2), ATTK(AT_CLAW, AD_PHYS, 1, 3), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(400, 50, MS_SILENT, MZ_SMALL), MR_COLD | MR_SLEEP | MR_POISON, 0, */
/*         M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_POIS | M1_OMNIVORE, */
/*         M2_UNDEAD | M2_WANDER | M2_HOSTILE, M3_INFRAVISION, CLR_BLACK), */
/*     MON("giant zombie", S_ZOMBIE, LVL(8, 8, 6, 0, -4), */
/*         (G_GENO | G_NOCORPSE | 1), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 2, 8), ATTK(AT_CLAW, AD_PHYS, 2, 8), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(2050, 375, MS_SILENT, MZ_HUGE), MR_COLD | MR_SLEEP | MR_POISON, 0, */
/*         M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID, */
/*         M2_UNDEAD | M2_STALK | M2_HOSTILE | M2_GIANT | M2_STRONG, */
/*         M3_INFRAVISION, CLR_CYAN), */
/*     MON("skeleton", S_ZOMBIE, LVL(12, 8, 4, 0, 0), (G_NOCORPSE | G_NOGEN), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 6), ATTK(AT_TUCH, AD_SLOW, 1, 6), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(300, 5, MS_BONES, MZ_HUMAN), */
/*         MR_COLD | MR_SLEEP | MR_POISON | MR_STONE, 0, */
/*         M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_THICK_HIDE, */
/*         M2_UNDEAD | M2_WANDER | M2_HOSTILE | M2_STRONG | M2_COLLECT */
/*             | M2_NASTY, */
/*         M3_INFRAVISION, CLR_WHITE), */
/*     #<{(| */
/*      * golems */
/*      |)}># */
/*     MON("straw golem", S_GOLEM, LVL(3, 12, 10, 0, 0), (G_NOCORPSE | 1), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 1, 2), ATTK(AT_CLAW, AD_PHYS, 1, 2), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(400, 0, MS_SILENT, MZ_LARGE), MR_COLD | MR_SLEEP | MR_POISON, 0, */
/*         M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID, M2_HOSTILE | M2_NEUTER, 0, */
/*         CLR_YELLOW), */
/*     MON("paper golem", S_GOLEM, LVL(3, 12, 10, 0, 0), (G_NOCORPSE | 1), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 1, 3), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(400, 0, MS_SILENT, MZ_LARGE), MR_COLD | MR_SLEEP | MR_POISON, 0, */
/*         M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID, M2_HOSTILE | M2_NEUTER, 0, */
/*         HI_PAPER), */
/*     MON("rope golem", S_GOLEM, LVL(4, 9, 8, 0, 0), (G_NOCORPSE | 1), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4), */
/*           ATTK(AT_HUGS, AD_PHYS, 6, 1), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(450, 0, MS_SILENT, MZ_LARGE), MR_SLEEP | MR_POISON, 0, */
/*         M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID, M2_HOSTILE | M2_NEUTER, 0, */
/*         CLR_BROWN), */
/*     MON("gold golem", S_GOLEM, LVL(5, 9, 6, 0, 0), (G_NOCORPSE | 1), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 2, 3), ATTK(AT_CLAW, AD_PHYS, 2, 3), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(450, 0, MS_SILENT, MZ_LARGE), MR_SLEEP | MR_POISON | MR_ACID, 0, */
/*         M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_THICK_HIDE, */
/*         M2_HOSTILE | M2_NEUTER, 0, HI_GOLD), */
/*     MON("leather golem", S_GOLEM, LVL(6, 6, 6, 0, 0), (G_NOCORPSE | 1), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 1, 6), ATTK(AT_CLAW, AD_PHYS, 1, 6), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(800, 0, MS_SILENT, MZ_LARGE), MR_SLEEP | MR_POISON, 0, */
/*         M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID, M2_HOSTILE | M2_NEUTER, 0, */
/*         HI_LEATHER), */
/*     MON("wood golem", S_GOLEM, LVL(7, 3, 4, 0, 0), (G_NOCORPSE | 1), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 3, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(900, 0, MS_SILENT, MZ_LARGE), MR_COLD | MR_SLEEP | MR_POISON, 0, */
/*         M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_THICK_HIDE, */
/*         M2_HOSTILE | M2_NEUTER, 0, HI_WOOD), */
/*     MON("flesh golem", S_GOLEM, LVL(9, 8, 9, 30, 0), (1), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 2, 8), ATTK(AT_CLAW, AD_PHYS, 2, 8), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1400, 600, MS_SILENT, MZ_LARGE), */
/*         MR_FIRE | MR_COLD | MR_ELEC | MR_SLEEP | MR_POISON, */
/*         MR_FIRE | MR_COLD | MR_ELEC | MR_SLEEP | MR_POISON, */
/*         M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID, M2_HOSTILE | M2_STRONG, 0, */
/*         CLR_RED), */
/*     MON("clay golem", S_GOLEM, LVL(11, 7, 7, 40, 0), (G_NOCORPSE | 1), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 3, 10), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(1550, 0, MS_SILENT, MZ_LARGE), MR_SLEEP | MR_POISON, 0, */
/*         M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_THICK_HIDE, */
/*         M2_HOSTILE | M2_STRONG, 0, CLR_BROWN), */
/*     MON("stone golem", S_GOLEM, LVL(14, 6, 5, 50, 0), (G_NOCORPSE | 1), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 3, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(1900, 0, MS_SILENT, MZ_LARGE), MR_SLEEP | MR_POISON | MR_STONE, 0, */
/*         M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_THICK_HIDE, */
/*         M2_HOSTILE | M2_STRONG, 0, CLR_GRAY), */
/*     MON("glass golem", S_GOLEM, LVL(16, 6, 1, 50, 0), (G_NOCORPSE | 1), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 2, 8), ATTK(AT_CLAW, AD_PHYS, 2, 8), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1800, 0, MS_SILENT, MZ_LARGE), MR_SLEEP | MR_POISON | MR_ACID, 0, */
/*         M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_THICK_HIDE, */
/*         M2_HOSTILE | M2_STRONG, 0, CLR_CYAN), */
/*     MON("iron golem", S_GOLEM, LVL(18, 6, 3, 60, 0), (G_NOCORPSE | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 4, 10), ATTK(AT_BREA, AD_DRST, 4, 6), */
/*           NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(2000, 0, MS_SILENT, MZ_LARGE), */
/*         MR_FIRE | MR_COLD | MR_ELEC | MR_SLEEP | MR_POISON, 0, */
/*         M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_THICK_HIDE | M1_POIS, */
/*         M2_HOSTILE | M2_STRONG | M2_COLLECT, 0, HI_METAL), */
/*     #<{(| */
/*      * humans, including elves and were-critters */
/*      |)}># */
/*     MON("human", S_HUMAN, LVL(0, 12, 10, 0, 0), G_NOGEN, #<{(| for corpses |)}># */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_COLLECT, M3_INFRAVISIBLE, */
/*         HI_DOMESTIC), */
/*     MON("wererat", S_HUMAN, LVL(2, 12, 10, 10, -7), (1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_WERE, MZ_HUMAN), MR_POISON, 0, */
/*         M1_HUMANOID | M1_POIS | M1_REGEN | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_WERE | M2_HOSTILE | M2_HUMAN | M2_COLLECT, */
/*         M3_INFRAVISIBLE, CLR_BROWN), */
/*     MON("werejackal", S_HUMAN, LVL(2, 12, 10, 10, -7), (1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_WERE, MZ_HUMAN), MR_POISON, 0, */
/*         M1_HUMANOID | M1_POIS | M1_REGEN | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_WERE | M2_HOSTILE | M2_HUMAN | M2_COLLECT, */
/*         M3_INFRAVISIBLE, CLR_RED), */
/*     MON("werewolf", S_HUMAN, LVL(5, 12, 10, 20, -7), (1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_WERE, MZ_HUMAN), MR_POISON, 0, */
/*         M1_HUMANOID | M1_POIS | M1_REGEN | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_WERE | M2_HOSTILE | M2_HUMAN | M2_COLLECT, */
/*         M3_INFRAVISIBLE, CLR_ORANGE), */
/*     MON("elf", S_HUMAN, LVL(10, 12, 10, 2, -3), G_NOGEN, #<{(| for corpses |)}># */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_ELF, 350, MS_HUMANOID, MZ_HUMAN), MR_SLEEP, MR_SLEEP, */
/*         M1_HUMANOID | M1_OMNIVORE | M1_SEE_INVIS, */
/*         M2_NOPOLY | M2_ELF | M2_STRONG | M2_COLLECT, */
/*         M3_INFRAVISION | M3_INFRAVISIBLE, HI_DOMESTIC), */
/*     MON("Woodland-elf", S_HUMAN, LVL(4, 12, 10, 10, -5), */
/*         (G_GENO | G_SGROUP | 2), A(ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK, */
/*                                    NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_ELF, 350, MS_HUMANOID, MZ_HUMAN), MR_SLEEP, MR_SLEEP, */
/*         M1_HUMANOID | M1_OMNIVORE | M1_SEE_INVIS, M2_ELF | M2_COLLECT, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_GREEN), */
/*     MON("Green-elf", S_HUMAN, LVL(5, 12, 10, 10, -6), (G_GENO | G_SGROUP | 2), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_ELF, 350, MS_HUMANOID, MZ_HUMAN), MR_SLEEP, MR_SLEEP, */
/*         M1_HUMANOID | M1_OMNIVORE | M1_SEE_INVIS, M2_ELF | M2_COLLECT, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BRIGHT_GREEN), */
/*     MON("Grey-elf", S_HUMAN, LVL(6, 12, 10, 10, -7), (G_GENO | G_SGROUP | 2), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_ELF, 350, MS_HUMANOID, MZ_HUMAN), MR_SLEEP, MR_SLEEP, */
/*         M1_HUMANOID | M1_OMNIVORE | M1_SEE_INVIS, M2_ELF | M2_COLLECT, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_GRAY), */
/*     MON("elf-lord", S_HUMAN, LVL(8, 12, 10, 20, -9), (G_GENO | G_SGROUP | 2), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 4), ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_ELF, 350, MS_HUMANOID, MZ_HUMAN), MR_SLEEP, MR_SLEEP, */
/*         M1_HUMANOID | M1_OMNIVORE | M1_SEE_INVIS, */
/*         M2_ELF | M2_STRONG | M2_LORD | M2_MALE | M2_COLLECT, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BRIGHT_BLUE), */
/*     MON("Elvenking", S_HUMAN, LVL(9, 12, 10, 25, -10), (G_GENO | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 4), ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_ELF, 350, MS_HUMANOID, MZ_HUMAN), MR_SLEEP, MR_SLEEP, */
/*         M1_HUMANOID | M1_OMNIVORE | M1_SEE_INVIS, */
/*         M2_ELF | M2_STRONG | M2_PRINCE | M2_MALE | M2_COLLECT, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, HI_LORD), */
/*     MON("doppelganger", S_HUMAN, LVL(9, 12, 5, 20, 0), (G_GENO | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 12), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_IMITATE, MZ_HUMAN), MR_SLEEP, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_HOSTILE | M2_STRONG | M2_COLLECT */
/*             | M2_SHAPESHIFTER, */
/*         M3_INFRAVISIBLE, HI_DOMESTIC), */
/*     MON("shopkeeper", S_HUMAN, LVL(12, 18, 0, 50, 0), G_NOGEN, */
/*         A(ATTK(AT_WEAP, AD_PHYS, 4, 4), ATTK(AT_WEAP, AD_PHYS, 4, 4), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_SELL, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, M2_NOPOLY | M2_HUMAN | M2_PEACEFUL */
/*                                        | M2_STRONG | M2_COLLECT | M2_MAGIC, */
/*         M3_INFRAVISIBLE, HI_DOMESTIC), */
/*     MON("guard", S_HUMAN, LVL(12, 12, 10, 40, 10), G_NOGEN, */
/*         A(ATTK(AT_WEAP, AD_PHYS, 4, 10), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_GUARD, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_MERC | M2_PEACEFUL | M2_STRONG | M2_COLLECT, */
/*         M3_INFRAVISIBLE, CLR_BLUE), */
/*     MON("prisoner", S_HUMAN, LVL(12, 12, 10, 0, 0), */
/*         G_NOGEN, #<{(| for special levels |)}># */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_DJINNI, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_COLLECT, */
/*         M3_INFRAVISIBLE | M3_CLOSE, HI_DOMESTIC), */
/*     MON("Oracle", S_HUMAN, LVL(12, 0, 0, 50, 0), (G_NOGEN | G_UNIQ), */
/*         A(ATTK(AT_NONE, AD_MAGM, 0, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_ORACLE, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_FEMALE, M3_INFRAVISIBLE, */
/*         HI_ZAP), */
/*     #<{(| aligned priests always have the epri extension attached; */
/*        individual instantiations should always have either ispriest */
/*        or isminion set |)}># */
/*     MON("aligned priest", S_HUMAN, LVL(12, 12, 10, 50, 0), G_NOGEN, */
/*         A(ATTK(AT_WEAP, AD_PHYS, 4, 10), ATTK(AT_KICK, AD_PHYS, 1, 4), */
/*           ATTK(AT_MAGC, AD_CLRC, 0, 0), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_PRIEST, MZ_HUMAN), MR_ELEC, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_LORD | M2_PEACEFUL | M2_COLLECT, */
/*         M3_INFRAVISIBLE, CLR_WHITE), */
/*     #<{(| high priests always have epri and always have ispriest set |)}># */
/*     MON("high priest", S_HUMAN, LVL(25, 15, 7, 70, 0), (G_NOGEN | G_UNIQ), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 4, 10), ATTK(AT_KICK, AD_PHYS, 2, 8), */
/*           ATTK(AT_MAGC, AD_CLRC, 2, 8), ATTK(AT_MAGC, AD_CLRC, 2, 8), NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_PRIEST, MZ_HUMAN), */
/*         MR_FIRE | MR_ELEC | MR_SLEEP | MR_POISON, 0, */
/*         M1_HUMANOID | M1_SEE_INVIS | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_MINION | M2_PRINCE | M2_NASTY | M2_COLLECT */
/*             | M2_MAGIC, */
/*         M3_INFRAVISIBLE, CLR_WHITE), */
/*     MON("soldier", S_HUMAN, LVL(6, 10, 10, 0, -2), (G_SGROUP | G_GENO | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_SOLDIER, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, M2_NOPOLY | M2_HUMAN | M2_MERC | M2_STALK */
/*                                        | M2_HOSTILE | M2_STRONG | M2_COLLECT, */
/*         M3_INFRAVISIBLE, CLR_GRAY), */
/*     MON("sergeant", S_HUMAN, LVL(8, 10, 10, 5, -3), (G_SGROUP | G_GENO | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_SOLDIER, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, M2_NOPOLY | M2_HUMAN | M2_MERC | M2_STALK */
/*                                        | M2_HOSTILE | M2_STRONG | M2_COLLECT, */
/*         M3_INFRAVISIBLE, CLR_RED), */
/*     MON("nurse", S_HUMAN, LVL(11, 6, 0, 0, 0), (G_GENO | 3), */
/*         A(ATTK(AT_CLAW, AD_HEAL, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_NURSE, MZ_HUMAN), MR_POISON, MR_POISON, */
/*         M1_HUMANOID | M1_OMNIVORE, M2_NOPOLY | M2_HUMAN | M2_HOSTILE, */
/*         M3_INFRAVISIBLE, HI_DOMESTIC), */
/*     MON("lieutenant", S_HUMAN, LVL(10, 10, 10, 15, -4), (G_GENO | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 3, 4), ATTK(AT_WEAP, AD_PHYS, 3, 4), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_SOLDIER, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, M2_NOPOLY | M2_HUMAN | M2_MERC | M2_STALK */
/*                                        | M2_HOSTILE | M2_STRONG | M2_COLLECT, */
/*         M3_INFRAVISIBLE, CLR_GREEN), */
/*     MON("captain", S_HUMAN, LVL(12, 10, 10, 15, -5), (G_GENO | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 4, 4), ATTK(AT_WEAP, AD_PHYS, 4, 4), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_SOLDIER, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, M2_NOPOLY | M2_HUMAN | M2_MERC | M2_STALK */
/*                                        | M2_HOSTILE | M2_STRONG | M2_COLLECT, */
/*         M3_INFRAVISIBLE, CLR_BLUE), */
/*     #<{(| Keep these separate - some of the mkroom code assumes that */
/*      * all the soldiers are contiguous. */
/*      |)}># */
/*     MON("watchman", S_HUMAN, LVL(6, 10, 10, 0, -2), */
/*         (G_SGROUP | G_NOGEN | G_GENO | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_SOLDIER, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, M2_NOPOLY | M2_HUMAN | M2_MERC | M2_STALK */
/*                                        | M2_PEACEFUL | M2_STRONG | M2_COLLECT, */
/*         M3_INFRAVISIBLE, CLR_GRAY), */
/*     MON("watch captain", S_HUMAN, LVL(10, 10, 10, 15, -4), */
/*         (G_NOGEN | G_GENO | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 3, 4), ATTK(AT_WEAP, AD_PHYS, 3, 4), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_SOLDIER, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, M2_NOPOLY | M2_HUMAN | M2_MERC | M2_STALK */
/*                                        | M2_PEACEFUL | M2_STRONG | M2_COLLECT, */
/*         M3_INFRAVISIBLE, CLR_GREEN), */
/*     #<{(| Unique humans not tied to quests. */
/*      |)}># */
/*     MON("Medusa", S_HUMAN, LVL(20, 12, 2, 50, -15), (G_NOGEN | G_UNIQ), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 4), ATTK(AT_CLAW, AD_PHYS, 1, 8), */
/*           ATTK(AT_GAZE, AD_STON, 0, 0), ATTK(AT_BITE, AD_DRST, 1, 6), NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_HISS, MZ_LARGE), MR_POISON | MR_STONE, */
/*         MR_POISON | MR_STONE, M1_FLY | M1_SWIM | M1_AMPHIBIOUS | M1_HUMANOID */
/*                                   | M1_POIS | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HOSTILE | M2_STRONG | M2_PNAME | M2_FEMALE, */
/*         M3_WAITFORU | M3_INFRAVISIBLE, CLR_BRIGHT_GREEN), */
/*     MON("Wizard of Yendor", S_HUMAN, LVL(30, 12, -8, 100, A_NONE), */
/*         (G_NOGEN | G_UNIQ), */
/*         A(ATTK(AT_CLAW, AD_SAMU, 2, 12), ATTK(AT_MAGC, AD_SPEL, 0, 0), */
/*           NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_CUSS, MZ_HUMAN), MR_FIRE | MR_POISON, */
/*         MR_FIRE | MR_POISON, */
/*         M1_FLY | M1_BREATHLESS | M1_HUMANOID | M1_REGEN | M1_SEE_INVIS */
/*             | M1_TPORT | M1_TPORT_CNTRL | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_HOSTILE | M2_STRONG | M2_NASTY | M2_PRINCE */
/*             | M2_MALE | M2_MAGIC, */
/*         M3_COVETOUS | M3_WAITFORU | M3_INFRAVISIBLE, HI_LORD), */
/*     MON("Croesus", S_HUMAN, LVL(20, 15, 0, 40, 15), (G_UNIQ | G_NOGEN), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 4, 10), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_GUARD, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_SEE_INVIS | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_STALK | M2_HOSTILE | M2_STRONG | M2_NASTY */
/*             | M2_PNAME | M2_PRINCE | M2_MALE | M2_GREEDY | M2_JEWELS */
/*             | M2_COLLECT | M2_MAGIC, */
/*         M3_INFRAVISIBLE, HI_LORD), */
/* #ifdef CHARON */
/*     MON("Charon", S_HUMAN, LVL(76, 18, -5, 120, 0), */
/*         (G_HELL | G_NOCORPSE | G_NOGEN | G_UNIQ), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 8), ATTK(AT_TUCH, AD_PLYS, 1, 8), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_FERRY, MZ_HUMAN), */
/*         MR_FIRE | MR_COLD | MR_POISON | MR_STONE, 0, */
/*         M1_BREATHLESS | M1_SEE_INVIS | M1_HUMANOID, */
/*         M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_PNAME | M2_MALE | M2_GREEDY */
/*             | M2_COLLECT, */
/*         M3_INFRAVISIBLE, CLR_WHITE), */
/* #endif */
/*     #<{(| */
/*      * ghosts */
/*      |)}># */
/*     MON("ghost", S_GHOST, LVL(10, 3, -5, 50, -5), (G_NOCORPSE | G_NOGEN), */
/*         A(ATTK(AT_TUCH, AD_PHYS, 1, 1), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 0, MS_SILENT, MZ_HUMAN), */
/*         MR_COLD | MR_DISINT | MR_SLEEP | MR_POISON | MR_STONE, 0, */
/*         M1_FLY | M1_BREATHLESS | M1_WALLWALK | M1_HUMANOID | M1_UNSOLID, */
/*         M2_NOPOLY | M2_UNDEAD | M2_STALK | M2_HOSTILE, M3_INFRAVISION, */
/*         CLR_GRAY), */
/*     MON("shade", S_GHOST, LVL(12, 10, 10, 0, 0), (G_NOCORPSE | G_NOGEN), */
/*         A(ATTK(AT_TUCH, AD_PLYS, 2, 6), ATTK(AT_TUCH, AD_SLOW, 1, 6), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 0, MS_WAIL, MZ_HUMAN), */
/*         MR_COLD | MR_DISINT | MR_SLEEP | MR_POISON | MR_STONE, 0, */
/*         M1_FLY | M1_BREATHLESS | M1_WALLWALK | M1_HUMANOID | M1_UNSOLID */
/*             | M1_SEE_INVIS, */
/*         M2_NOPOLY | M2_UNDEAD | M2_WANDER | M2_STALK | M2_HOSTILE | M2_NASTY, */
/*         M3_INFRAVISION, CLR_BLACK), */
/*     #<{(| */
/*      * (major) demons */
/*      |)}># */
/*     MON("water demon", S_DEMON, LVL(8, 12, -4, 30, -7), */
/*         (G_NOCORPSE | G_NOGEN), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 3), ATTK(AT_CLAW, AD_PHYS, 1, 3), */
/*           ATTK(AT_BITE, AD_PHYS, 1, 3), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_DJINNI, MZ_HUMAN), MR_FIRE | MR_POISON, 0, */
/*         M1_HUMANOID | M1_POIS | M1_SWIM, */
/*         M2_NOPOLY | M2_DEMON | M2_STALK | M2_HOSTILE | M2_NASTY | M2_COLLECT, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BLUE), */
/*     #<{(| standard demons & devils */
/*      |)}># */
/* #define SEDUCTION_ATTACKS_YES                                     \ */
/*     A(ATTK(AT_BITE, AD_SSEX, 0, 0), ATTK(AT_CLAW, AD_PHYS, 1, 3), \ */
/*       ATTK(AT_CLAW, AD_PHYS, 1, 3), NO_ATTK, NO_ATTK, NO_ATTK) */
/* #define SEDUCTION_ATTACKS_NO                                      \ */
/*     A(ATTK(AT_CLAW, AD_PHYS, 1, 3), ATTK(AT_CLAW, AD_PHYS, 1, 3), \ */
/*       ATTK(AT_BITE, AD_DRLI, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK) */
/*     MON("succubus", S_DEMON, LVL(6, 12, 0, 70, -9), (G_NOCORPSE | 1), */
/*         SEDUCTION_ATTACKS_YES, SIZ(WT_HUMAN, 400, MS_SEDUCE, MZ_HUMAN), */
/*         MR_FIRE | MR_POISON, 0, M1_HUMANOID | M1_FLY | M1_POIS, */
/*         M2_DEMON | M2_STALK | M2_HOSTILE | M2_NASTY | M2_FEMALE, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_GRAY), */
/*     MON("horned devil", S_DEMON, LVL(6, 9, -5, 50, 11), */
/*         (G_HELL | G_NOCORPSE | 2), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4), */
/*           ATTK(AT_BITE, AD_PHYS, 2, 3), ATTK(AT_STNG, AD_PHYS, 1, 3), NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_SILENT, MZ_HUMAN), MR_FIRE | MR_POISON, 0, */
/*         M1_POIS | M1_THICK_HIDE, M2_DEMON | M2_STALK | M2_HOSTILE | M2_NASTY, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BROWN), */
/*     MON("incubus", S_DEMON, LVL(6, 12, 0, 70, -9), (G_NOCORPSE | 1), */
/*         SEDUCTION_ATTACKS_YES, SIZ(WT_HUMAN, 400, MS_SEDUCE, MZ_HUMAN), */
/*         MR_FIRE | MR_POISON, 0, M1_HUMANOID | M1_FLY | M1_POIS, */
/*         M2_DEMON | M2_STALK | M2_HOSTILE | M2_NASTY | M2_MALE, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_GRAY), */
/*     #<{(| Used by AD&D for a type of demon, originally one of the Furies */
/*        and spelled this way |)}># */
/*     MON("erinys", S_DEMON, LVL(7, 12, 2, 30, 10), */
/*         (G_HELL | G_NOCORPSE | G_SGROUP | 2), */
/*         A(ATTK(AT_WEAP, AD_DRST, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_SILENT, MZ_HUMAN), MR_FIRE | MR_POISON, 0, */
/*         M1_HUMANOID | M1_POIS, */
/*         M2_NOPOLY | M2_DEMON | M2_STALK | M2_HOSTILE | M2_STRONG | M2_NASTY */
/*             | M2_FEMALE | M2_COLLECT, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED), */
/*     MON("barbed devil", S_DEMON, LVL(8, 12, 0, 35, 8), */
/*         (G_HELL | G_NOCORPSE | G_SGROUP | 2), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 2, 4), ATTK(AT_CLAW, AD_PHYS, 2, 4), */
/*           ATTK(AT_STNG, AD_PHYS, 3, 4), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_SILENT, MZ_HUMAN), MR_FIRE | MR_POISON, 0, */
/*         M1_POIS | M1_THICK_HIDE, M2_DEMON | M2_STALK | M2_HOSTILE | M2_NASTY, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED), */
/*     MON("marilith", S_DEMON, LVL(7, 12, -6, 80, -12), */
/*         (G_HELL | G_NOCORPSE | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 4), ATTK(AT_WEAP, AD_PHYS, 2, 4), */
/*           ATTK(AT_CLAW, AD_PHYS, 2, 4), ATTK(AT_CLAW, AD_PHYS, 2, 4), */
/*           ATTK(AT_CLAW, AD_PHYS, 2, 4), ATTK(AT_CLAW, AD_PHYS, 2, 4)), */
/*         SIZ(WT_HUMAN, 400, MS_CUSS, MZ_LARGE), MR_FIRE | MR_POISON, 0, */
/*         M1_HUMANOID | M1_SLITHY | M1_SEE_INVIS | M1_POIS, */
/*         M2_DEMON | M2_STALK | M2_HOSTILE | M2_NASTY | M2_FEMALE | M2_COLLECT, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED), */
/*     MON("vrock", S_DEMON, LVL(8, 12, 0, 50, -9), */
/*         (G_HELL | G_NOCORPSE | G_SGROUP | 2), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4), */
/*           ATTK(AT_CLAW, AD_PHYS, 1, 8), ATTK(AT_CLAW, AD_PHYS, 1, 8), */
/*           ATTK(AT_BITE, AD_PHYS, 1, 6), NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_SILENT, MZ_LARGE), MR_FIRE | MR_POISON, 0, */
/*         M1_POIS, M2_DEMON | M2_STALK | M2_HOSTILE | M2_NASTY, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED), */
/*     MON("hezrou", S_DEMON, LVL(9, 6, -2, 55, -10), */
/*         (G_HELL | G_NOCORPSE | G_SGROUP | 2), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 1, 3), ATTK(AT_CLAW, AD_PHYS, 1, 3), */
/*           ATTK(AT_BITE, AD_PHYS, 4, 4), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_SILENT, MZ_LARGE), MR_FIRE | MR_POISON, 0, */
/*         M1_HUMANOID | M1_POIS, M2_DEMON | M2_STALK | M2_HOSTILE | M2_NASTY, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED), */
/*     MON("bone devil", S_DEMON, LVL(9, 15, -1, 40, -9), */
/*         (G_HELL | G_NOCORPSE | G_SGROUP | 2), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 3, 4), ATTK(AT_STNG, AD_DRST, 2, 4), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_SILENT, MZ_LARGE), MR_FIRE | MR_POISON, 0, */
/*         M1_POIS, M2_DEMON | M2_STALK | M2_HOSTILE | M2_NASTY | M2_COLLECT, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_GRAY), */
/*     MON("ice devil", S_DEMON, LVL(11, 6, -4, 55, -12), */
/*         (G_HELL | G_NOCORPSE | 2), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4), */
/*           ATTK(AT_BITE, AD_PHYS, 2, 4), ATTK(AT_STNG, AD_COLD, 3, 4), NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_SILENT, MZ_LARGE), */
/*         MR_FIRE | MR_COLD | MR_POISON, 0, M1_SEE_INVIS | M1_POIS, */
/*         M2_DEMON | M2_STALK | M2_HOSTILE | M2_NASTY, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_WHITE), */
/*     MON("nalfeshnee", S_DEMON, LVL(11, 9, -1, 65, -11), */
/*         (G_HELL | G_NOCORPSE | 1), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4), */
/*           ATTK(AT_BITE, AD_PHYS, 2, 4), ATTK(AT_MAGC, AD_SPEL, 0, 0), NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_SPELL, MZ_LARGE), MR_FIRE | MR_POISON, 0, */
/*         M1_HUMANOID | M1_POIS, M2_DEMON | M2_STALK | M2_HOSTILE | M2_NASTY, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED), */
/*     MON("pit fiend", S_DEMON, LVL(13, 6, -3, 65, -13), */
/*         (G_HELL | G_NOCORPSE | 2), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 4, 2), ATTK(AT_WEAP, AD_PHYS, 4, 2), */
/*           ATTK(AT_HUGS, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_GROWL, MZ_LARGE), MR_FIRE | MR_POISON, 0, */
/*         M1_SEE_INVIS | M1_POIS, */
/*         M2_DEMON | M2_STALK | M2_HOSTILE | M2_NASTY | M2_COLLECT, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED), */
/*     MON("sandestin", S_DEMON, LVL(13, 12, 4, 60, -5), */
/*         (G_HELL | G_NOCORPSE | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 6), ATTK(AT_WEAP, AD_PHYS, 2, 6), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1500, 400, MS_CUSS, MZ_HUMAN), MR_STONE, 0, M1_HUMANOID, */
/*         M2_NOPOLY | M2_STALK | M2_STRONG | M2_COLLECT | M2_SHAPESHIFTER, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_GRAY), */
/*     MON("balrog", S_DEMON, LVL(16, 5, -2, 75, -14), (G_HELL | G_NOCORPSE | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 8, 4), ATTK(AT_WEAP, AD_PHYS, 4, 6), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_SILENT, MZ_LARGE), MR_FIRE | MR_POISON, 0, */
/*         M1_FLY | M1_SEE_INVIS | M1_POIS, */
/*         M2_DEMON | M2_STALK | M2_HOSTILE | M2_STRONG | M2_NASTY | M2_COLLECT, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED), */
/*     #<{(| Named demon lords & princes plus Arch-Devils. */
/*      * (their order matters; see minion.c) */
/*      |)}># */
/*     MON("Juiblex", S_DEMON, LVL(50, 3, -7, 65, -15), */
/*         (G_HELL | G_NOCORPSE | G_NOGEN | G_UNIQ), */
/*         A(ATTK(AT_ENGL, AD_DISE, 4, 10), ATTK(AT_SPIT, AD_ACID, 3, 6), */
/*           NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1500, 0, MS_GURGLE, MZ_LARGE), */
/*         MR_FIRE | MR_POISON | MR_ACID | MR_STONE, 0, */
/*         M1_AMPHIBIOUS | M1_AMORPHOUS | M1_NOHEAD | M1_FLY | M1_SEE_INVIS */
/*             | M1_ACID | M1_POIS, */
/*         M2_NOPOLY | M2_DEMON | M2_STALK | M2_HOSTILE | M2_PNAME | M2_NASTY */
/*             | M2_LORD | M2_MALE, */
/*         M3_WAITFORU | M3_WANTSAMUL | M3_INFRAVISION, CLR_BRIGHT_GREEN), */
/*     MON("Yeenoghu", S_DEMON, LVL(56, 18, -5, 80, -15), */
/*         (G_HELL | G_NOCORPSE | G_NOGEN | G_UNIQ), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 3, 6), ATTK(AT_WEAP, AD_CONF, 2, 8), */
/*           ATTK(AT_CLAW, AD_PLYS, 1, 6), ATTK(AT_MAGC, AD_MAGM, 2, 6), NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(900, 500, MS_ORC, MZ_LARGE), MR_FIRE | MR_POISON, 0, */
/*         M1_FLY | M1_SEE_INVIS | M1_POIS, */
/*         M2_NOPOLY | M2_DEMON | M2_STALK | M2_HOSTILE | M2_PNAME | M2_NASTY */
/*             | M2_LORD | M2_MALE | M2_COLLECT, */
/*         M3_WANTSAMUL | M3_INFRAVISIBLE | M3_INFRAVISION, HI_LORD), */
/*     MON("Orcus", S_DEMON, LVL(66, 9, -6, 85, -20), */
/*         (G_HELL | G_NOCORPSE | G_NOGEN | G_UNIQ), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 3, 6), ATTK(AT_CLAW, AD_PHYS, 3, 4), */
/*           ATTK(AT_CLAW, AD_PHYS, 3, 4), ATTK(AT_MAGC, AD_SPEL, 8, 6), */
/*           ATTK(AT_STNG, AD_DRST, 2, 4), NO_ATTK), */
/*         SIZ(1500, 500, MS_ORC, MZ_HUGE), MR_FIRE | MR_POISON, 0, */
/*         M1_FLY | M1_SEE_INVIS | M1_POIS, */
/*         M2_NOPOLY | M2_DEMON | M2_STALK | M2_HOSTILE | M2_PNAME | M2_NASTY */
/*             | M2_PRINCE | M2_MALE | M2_COLLECT, */
/*         M3_WAITFORU | M3_WANTSBOOK | M3_WANTSAMUL | M3_INFRAVISIBLE */
/*             | M3_INFRAVISION, */
/*         HI_LORD), */
/*     MON("Geryon", S_DEMON, LVL(72, 3, -3, 75, 15), */
/*         (G_HELL | G_NOCORPSE | G_NOGEN | G_UNIQ), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 3, 6), ATTK(AT_CLAW, AD_PHYS, 3, 6), */
/*           ATTK(AT_STNG, AD_DRST, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1500, 500, MS_BRIBE, MZ_HUGE), MR_FIRE | MR_POISON, 0, */
/*         M1_FLY | M1_SEE_INVIS | M1_POIS | M1_SLITHY, */
/*         M2_NOPOLY | M2_DEMON | M2_STALK | M2_HOSTILE | M2_PNAME | M2_NASTY */
/*             | M2_PRINCE | M2_MALE, */
/*         M3_WANTSAMUL | M3_INFRAVISIBLE | M3_INFRAVISION, HI_LORD), */
/*     MON("Dispater", S_DEMON, LVL(78, 15, -2, 80, 15), */
/*         (G_HELL | G_NOCORPSE | G_NOGEN | G_UNIQ), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 4, 6), ATTK(AT_MAGC, AD_SPEL, 6, 6), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1500, 500, MS_BRIBE, MZ_HUMAN), MR_FIRE | MR_POISON, 0, */
/*         M1_FLY | M1_SEE_INVIS | M1_POIS | M1_HUMANOID, */
/*         M2_NOPOLY | M2_DEMON | M2_STALK | M2_HOSTILE | M2_PNAME | M2_NASTY */
/*             | M2_PRINCE | M2_MALE | M2_COLLECT, */
/*         M3_WANTSAMUL | M3_INFRAVISIBLE | M3_INFRAVISION, HI_LORD), */
/*     MON("Baalzebub", S_DEMON, LVL(89, 9, -5, 85, 20), */
/*         (G_HELL | G_NOCORPSE | G_NOGEN | G_UNIQ), */
/*         A(ATTK(AT_BITE, AD_DRST, 2, 6), ATTK(AT_GAZE, AD_STUN, 2, 6), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1500, 500, MS_BRIBE, MZ_LARGE), MR_FIRE | MR_POISON, 0, */
/*         M1_FLY | M1_SEE_INVIS | M1_POIS, */
/*         M2_NOPOLY | M2_DEMON | M2_STALK | M2_HOSTILE | M2_PNAME | M2_NASTY */
/*             | M2_PRINCE | M2_MALE, */
/*         M3_WANTSAMUL | M3_WAITFORU | M3_INFRAVISIBLE | M3_INFRAVISION, */
/*         HI_LORD), */
/*     MON("Asmodeus", S_DEMON, LVL(105, 12, -7, 90, 20), */
/*         (G_HELL | G_NOCORPSE | G_NOGEN | G_UNIQ), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 4, 4), ATTK(AT_MAGC, AD_COLD, 6, 6), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1500, 500, MS_BRIBE, MZ_HUGE), MR_FIRE | MR_COLD | MR_POISON, 0, */
/*         M1_FLY | M1_SEE_INVIS | M1_HUMANOID | M1_POIS, */
/*         M2_NOPOLY | M2_DEMON | M2_STALK | M2_HOSTILE | M2_PNAME | M2_STRONG */
/*             | M2_NASTY | M2_PRINCE | M2_MALE, */
/*         M3_WANTSAMUL | M3_WAITFORU | M3_INFRAVISIBLE | M3_INFRAVISION, */
/*         HI_LORD), */
/*     MON("Demogorgon", S_DEMON, LVL(106, 15, -8, 95, -20), */
/*         (G_HELL | G_NOCORPSE | G_NOGEN | G_UNIQ), */
/*         A(ATTK(AT_MAGC, AD_SPEL, 8, 6), ATTK(AT_STNG, AD_DRLI, 1, 4), */
/*           ATTK(AT_CLAW, AD_DISE, 1, 6), ATTK(AT_CLAW, AD_DISE, 1, 6), NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(1500, 500, MS_GROWL, MZ_HUGE), MR_FIRE | MR_POISON, 0, */
/*         M1_FLY | M1_SEE_INVIS | M1_NOHANDS | M1_POIS, */
/*         M2_NOPOLY | M2_DEMON | M2_STALK | M2_HOSTILE | M2_PNAME | M2_NASTY */
/*             | M2_PRINCE | M2_MALE, */
/*         M3_WANTSAMUL | M3_INFRAVISIBLE | M3_INFRAVISION, HI_LORD), */
/*     #<{(| Riders -- the Four Horsemen of the Apocalypse ("War" == player); */
/*      * depicted with '&' but do not have M2_DEMON set. */
/*      |)}># */
/*     MON("Death", S_DEMON, LVL(30, 12, -5, 100, 0), (G_UNIQ | G_NOGEN), */
/*         A(ATTK(AT_TUCH, AD_DETH, 8, 8), ATTK(AT_TUCH, AD_DETH, 8, 8), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 1, MS_RIDER, MZ_HUMAN), */
/*         MR_FIRE | MR_COLD | MR_ELEC | MR_SLEEP | MR_POISON | MR_STONE, 0, */
/*         M1_FLY | M1_HUMANOID | M1_REGEN | M1_SEE_INVIS | M1_TPORT_CNTRL, */
/*         M2_NOPOLY | M2_STALK | M2_HOSTILE | M2_PNAME | M2_STRONG | M2_NASTY, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION | M3_DISPLACES, HI_LORD), */
/*     MON("Pestilence", S_DEMON, LVL(30, 12, -5, 100, 0), (G_UNIQ | G_NOGEN), */
/*         A(ATTK(AT_TUCH, AD_PEST, 8, 8), ATTK(AT_TUCH, AD_PEST, 8, 8), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 1, MS_RIDER, MZ_HUMAN), */
/*         MR_FIRE | MR_COLD | MR_ELEC | MR_SLEEP | MR_POISON | MR_STONE, 0, */
/*         M1_FLY | M1_HUMANOID | M1_REGEN | M1_SEE_INVIS | M1_TPORT_CNTRL, */
/*         M2_NOPOLY | M2_STALK | M2_HOSTILE | M2_PNAME | M2_STRONG | M2_NASTY, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION | M3_DISPLACES, HI_LORD), */
/*     MON("Famine", S_DEMON, LVL(30, 12, -5, 100, 0), (G_UNIQ | G_NOGEN), */
/*         A(ATTK(AT_TUCH, AD_FAMN, 8, 8), ATTK(AT_TUCH, AD_FAMN, 8, 8), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 1, MS_RIDER, MZ_HUMAN), */
/*         MR_FIRE | MR_COLD | MR_ELEC | MR_SLEEP | MR_POISON | MR_STONE, 0, */
/*         M1_FLY | M1_HUMANOID | M1_REGEN | M1_SEE_INVIS | M1_TPORT_CNTRL, */
/*         M2_NOPOLY | M2_STALK | M2_HOSTILE | M2_PNAME | M2_STRONG | M2_NASTY, */
/*         M3_INFRAVISIBLE | M3_INFRAVISION | M3_DISPLACES, HI_LORD), */
/*     #<{(| other demons */
/*      |)}># */
/* #ifdef MAIL */
/*     MON("mail daemon", S_DEMON, LVL(56, 24, 10, 127, 0), */
/*         (G_NOGEN | G_NOCORPSE), */
/*         A(NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(600, 300, MS_SILENT, MZ_HUMAN), */
/*         MR_FIRE | MR_COLD | MR_ELEC | MR_SLEEP | MR_POISON | MR_STONE, 0, */
/*         M1_FLY | M1_SWIM | M1_BREATHLESS | M1_SEE_INVIS | M1_HUMANOID */
/*             | M1_POIS, */
/*         M2_NOPOLY | M2_STALK | M2_PEACEFUL, M3_INFRAVISIBLE | M3_INFRAVISION, */
/*         CLR_BRIGHT_BLUE), */
/* #endif */
/*     MON("djinni", S_DEMON, LVL(7, 12, 4, 30, 0), (G_NOGEN | G_NOCORPSE), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(1500, 400, MS_DJINNI, MZ_HUMAN), MR_POISON | MR_STONE, 0, */
/*         M1_HUMANOID | M1_FLY | M1_POIS, M2_NOPOLY | M2_STALK | M2_COLLECT, */
/*         M3_INFRAVISIBLE, CLR_YELLOW), */
/*     #<{(| */
/*      * sea monsters */
/*      |)}># */
/*     MON("jellyfish", S_EEL, LVL(3, 3, 6, 0, 0), (G_GENO | G_NOGEN), */
/*         A(ATTK(AT_STNG, AD_DRST, 3, 3), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(80, 20, MS_SILENT, MZ_SMALL), MR_POISON, MR_POISON, */
/*         M1_SWIM | M1_AMPHIBIOUS | M1_SLITHY | M1_NOLIMBS | M1_NOHEAD */
/*             | M1_NOTAKE | M1_POIS, */
/*         M2_HOSTILE, 0, CLR_BLUE), */
/*     MON("piranha", S_EEL, LVL(5, 12, 4, 0, 0), (G_GENO | G_NOGEN | G_SGROUP), */
/*         A(ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(60, 30, MS_SILENT, MZ_SMALL), 0, 0, */
/*         M1_SWIM | M1_AMPHIBIOUS | M1_ANIMAL | M1_SLITHY | M1_NOLIMBS */
/*             | M1_CARNIVORE | M1_OVIPAROUS | M1_NOTAKE, */
/*         M2_HOSTILE, 0, CLR_RED), */
/*     MON("shark", S_EEL, LVL(7, 12, 2, 0, 0), (G_GENO | G_NOGEN), */
/*         A(ATTK(AT_BITE, AD_PHYS, 5, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(500, 350, MS_SILENT, MZ_LARGE), 0, 0, */
/*         M1_SWIM | M1_AMPHIBIOUS | M1_ANIMAL | M1_SLITHY | M1_NOLIMBS */
/*             | M1_CARNIVORE | M1_OVIPAROUS | M1_THICK_HIDE | M1_NOTAKE, */
/*         M2_HOSTILE, 0, CLR_GRAY), */
/*     MON("giant eel", S_EEL, LVL(5, 9, -1, 0, 0), (G_GENO | G_NOGEN), */
/*         A(ATTK(AT_BITE, AD_PHYS, 3, 6), ATTK(AT_TUCH, AD_WRAP, 0, 0), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(200, 250, MS_SILENT, MZ_HUGE), 0, 0, */
/*         M1_SWIM | M1_AMPHIBIOUS | M1_ANIMAL | M1_SLITHY | M1_NOLIMBS */
/*             | M1_CARNIVORE | M1_OVIPAROUS | M1_NOTAKE, */
/*         M2_HOSTILE, M3_INFRAVISIBLE, CLR_CYAN), */
/*     MON("electric eel", S_EEL, LVL(7, 10, -3, 0, 0), (G_GENO | G_NOGEN), */
/*         A(ATTK(AT_BITE, AD_ELEC, 4, 6), ATTK(AT_TUCH, AD_WRAP, 0, 0), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(200, 250, MS_SILENT, MZ_HUGE), MR_ELEC, MR_ELEC, */
/*         M1_SWIM | M1_AMPHIBIOUS | M1_ANIMAL | M1_SLITHY | M1_NOLIMBS */
/*             | M1_CARNIVORE | M1_OVIPAROUS | M1_NOTAKE, */
/*         M2_HOSTILE, M3_INFRAVISIBLE, CLR_BRIGHT_BLUE), */
/*     MON("kraken", S_EEL, LVL(20, 3, 6, 0, -3), (G_GENO | G_NOGEN), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 2, 4), ATTK(AT_CLAW, AD_PHYS, 2, 4), */
/*           ATTK(AT_HUGS, AD_WRAP, 2, 6), ATTK(AT_BITE, AD_PHYS, 5, 4), NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(1800, 1000, MS_SILENT, MZ_HUGE), 0, 0, */
/*         M1_SWIM | M1_AMPHIBIOUS | M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, */
/*         M2_NOPOLY | M2_HOSTILE | M2_STRONG, M3_INFRAVISIBLE, CLR_RED), */
/*     #<{(| */
/*      * lizards, &c */
/*      |)}># */
/*     MON("newt", S_LIZARD, LVL(0, 6, 8, 0, 0), (G_GENO | 5), */
/*         A(ATTK(AT_BITE, AD_PHYS, 1, 2), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(10, 20, MS_SILENT, MZ_TINY), 0, 0, */
/*         M1_SWIM | M1_AMPHIBIOUS | M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, */
/*         M2_HOSTILE, 0, CLR_YELLOW), */
/*     MON("gecko", S_LIZARD, LVL(1, 6, 8, 0, 0), (G_GENO | 5), */
/*         A(ATTK(AT_BITE, AD_PHYS, 1, 3), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(10, 20, MS_SQEEK, MZ_TINY), 0, 0, */
/*         M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, 0, CLR_GREEN), */
/*     MON("iguana", S_LIZARD, LVL(2, 6, 7, 0, 0), (G_GENO | 5), */
/*         A(ATTK(AT_BITE, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(30, 30, MS_SILENT, MZ_TINY), 0, 0, */
/*         M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, 0, CLR_BROWN), */
/*     MON("baby crocodile", S_LIZARD, LVL(3, 6, 7, 0, 0), G_GENO, */
/*         A(ATTK(AT_BITE, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(200, 200, MS_SILENT, MZ_MEDIUM), 0, 0, */
/*         M1_SWIM | M1_AMPHIBIOUS | M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, */
/*         M2_HOSTILE, 0, CLR_BROWN), */
/*     MON("lizard", S_LIZARD, LVL(5, 6, 6, 10, 0), (G_GENO | 5), */
/*         A(ATTK(AT_BITE, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(10, 40, MS_SILENT, MZ_TINY), MR_STONE, MR_STONE, */
/*         M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, 0, CLR_GREEN), */
/*     MON("chameleon", S_LIZARD, LVL(6, 5, 6, 10, 0), (G_GENO | 2), */
/*         A(ATTK(AT_BITE, AD_PHYS, 4, 2), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(100, 100, MS_SILENT, MZ_TINY), 0, 0, */
/*         M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, */
/*         M2_NOPOLY | M2_HOSTILE | M2_SHAPESHIFTER, 0, CLR_BROWN), */
/*     MON("crocodile", S_LIZARD, LVL(6, 9, 5, 0, 0), (G_GENO | 1), */
/*         A(ATTK(AT_BITE, AD_PHYS, 4, 2), ATTK(AT_CLAW, AD_PHYS, 1, 12), */
/*           NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_SILENT, MZ_LARGE), 0, 0, */
/*         M1_SWIM | M1_AMPHIBIOUS | M1_ANIMAL | M1_THICK_HIDE | M1_NOHANDS */
/*             | M1_OVIPAROUS | M1_CARNIVORE, */
/*         M2_STRONG | M2_HOSTILE, 0, CLR_BROWN), */
/*     MON("salamander", S_LIZARD, LVL(8, 12, -1, 0, -9), (G_HELL | 1), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 8), ATTK(AT_TUCH, AD_FIRE, 1, 6), */
/*           ATTK(AT_HUGS, AD_PHYS, 2, 6), ATTK(AT_HUGS, AD_FIRE, 3, 6), NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(1500, 400, MS_MUMBLE, MZ_HUMAN), MR_SLEEP | MR_FIRE, MR_FIRE, */
/*         M1_HUMANOID | M1_SLITHY | M1_THICK_HIDE | M1_POIS, */
/*         M2_STALK | M2_HOSTILE | M2_COLLECT | M2_MAGIC, M3_INFRAVISIBLE, */
/*         CLR_ORANGE), */
/*  */
/*     #<{(| */
/*      * dummy monster needed for visual interface */
/*      * (marking it unique prevents figurines) */
/*      |)}># */
/*     MON("long worm tail", S_WORM_TAIL, LVL(0, 0, 0, 0, 0), */
/*         (G_NOGEN | G_NOCORPSE | G_UNIQ), */
/*         A(NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(0, 0, 0, 0), 0, 0, 0L, M2_NOPOLY, 0, CLR_BROWN), */
/*     #<{(| Note: */
/*      * Worm tail must be between the normal monsters and the special */
/*      * quest & pseudo-character ones because an optimization in the */
/*      * random monster selection code assumes everything beyond here */
/*      * has the G_NOGEN and M2_NOPOLY attributes. */
/*      |)}># */
/*  */
/*     #<{(| */
/*      * character classes */
/*      |)}># */
/*     MON("archeologist", S_HUMAN, LVL(10, 12, 10, 1, 3), G_NOGEN, */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_TUNNEL | M1_NEEDPICK | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_COLLECT, M3_INFRAVISIBLE, */
/*         HI_DOMESTIC), */
/*     MON("barbarian", S_HUMAN, LVL(10, 12, 10, 1, 0), G_NOGEN, */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), MR_POISON, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_COLLECT, M3_INFRAVISIBLE, */
/*         HI_DOMESTIC), */
/*     MON("caveman", S_HUMAN, LVL(10, 12, 10, 0, 1), G_NOGEN, */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_MALE | M2_COLLECT, */
/*         M3_INFRAVISIBLE, HI_DOMESTIC), */
/*     MON("cavewoman", S_HUMAN, LVL(10, 12, 10, 0, 1), G_NOGEN, */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_FEMALE | M2_COLLECT, */
/*         M3_INFRAVISIBLE, HI_DOMESTIC), */
/*     MON("healer", S_HUMAN, LVL(10, 12, 10, 1, 0), G_NOGEN, */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), MR_POISON, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_COLLECT, M3_INFRAVISIBLE, */
/*         HI_DOMESTIC), */
/*     MON("knight", S_HUMAN, LVL(10, 12, 10, 1, 3), G_NOGEN, */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_COLLECT, M3_INFRAVISIBLE, */
/*         HI_DOMESTIC), */
/*     MON("monk", S_HUMAN, LVL(10, 12, 10, 2, 0), G_NOGEN, */
/*         A(ATTK(AT_CLAW, AD_PHYS, 1, 8), ATTK(AT_KICK, AD_PHYS, 1, 8), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_HERBIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_COLLECT | M2_MALE, */
/*         M3_INFRAVISIBLE, HI_DOMESTIC), */
/*     MON("priest", S_HUMAN, LVL(10, 12, 10, 2, 0), G_NOGEN, */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_MALE | M2_COLLECT, */
/*         M3_INFRAVISIBLE, HI_DOMESTIC), */
/*     MON("priestess", S_HUMAN, LVL(10, 12, 10, 2, 0), G_NOGEN, */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_FEMALE | M2_COLLECT, */
/*         M3_INFRAVISIBLE, HI_DOMESTIC), */
/*     MON("ranger", S_HUMAN, LVL(10, 12, 10, 2, -3), G_NOGEN, */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_COLLECT, M3_INFRAVISIBLE, */
/*         HI_DOMESTIC), */
/*     MON("rogue", S_HUMAN, LVL(10, 12, 10, 1, -3), G_NOGEN, */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_GREEDY | M2_JEWELS | M2_COLLECT, */
/*         M3_INFRAVISIBLE, HI_DOMESTIC), */
/*     MON("samurai", S_HUMAN, LVL(10, 12, 10, 1, 3), G_NOGEN, */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 8), ATTK(AT_WEAP, AD_PHYS, 1, 8), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_COLLECT, M3_INFRAVISIBLE, */
/*         HI_DOMESTIC), */
/*     MON("tourist", S_HUMAN, LVL(10, 12, 10, 1, 0), G_NOGEN, */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_COLLECT, M3_INFRAVISIBLE, */
/*         HI_DOMESTIC), */
/*     MON("valkyrie", S_HUMAN, LVL(10, 12, 10, 1, -1), G_NOGEN, */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 8), ATTK(AT_WEAP, AD_PHYS, 1, 8), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), MR_COLD, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_FEMALE | M2_COLLECT, */
/*         M3_INFRAVISIBLE, HI_DOMESTIC), */
/*     MON("wizard", S_HUMAN, LVL(10, 12, 10, 3, 0), G_NOGEN, */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_COLLECT | M2_MAGIC, */
/*         M3_INFRAVISIBLE, HI_DOMESTIC), */
/*     #<{(| */
/*      * quest leaders */
/*      |)}># */
/*     MON("Lord Carnarvon", S_HUMAN, LVL(20, 12, 0, 30, 20), (G_NOGEN | G_UNIQ), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_LEADER, MZ_HUMAN), 0, 0, */
/*         M1_TUNNEL | M1_NEEDPICK | M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_PNAME | M2_PEACEFUL | M2_STRONG | M2_MALE */
/*             | M2_COLLECT | M2_MAGIC, */
/*         M3_CLOSE | M3_INFRAVISIBLE, HI_LORD), */
/*     MON("Pelias", S_HUMAN, LVL(20, 12, 0, 30, 0), (G_NOGEN | G_UNIQ), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_LEADER, MZ_HUMAN), MR_POISON, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_PNAME | M2_PEACEFUL | M2_STRONG | M2_MALE */
/*             | M2_COLLECT | M2_MAGIC, */
/*         M3_CLOSE | M3_INFRAVISIBLE, HI_LORD), */
/*     MON("Shaman Karnov", S_HUMAN, LVL(20, 12, 0, 30, 20), (G_NOGEN | G_UNIQ), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_LEADER, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_PNAME | M2_PEACEFUL | M2_STRONG | M2_MALE */
/*             | M2_COLLECT | M2_MAGIC, */
/*         M3_CLOSE | M3_INFRAVISIBLE, HI_LORD), */
/* #if 0 #<{(| OBSOLETE |)}># */
/*     #<{(| Two for elves - one of each sex. */
/*      |)}># */
/*     MON("Earendil", S_HUMAN, */
/*         LVL(20, 12, 0, 50, -20), (G_NOGEN | G_UNIQ), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 8), */
/*           NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_ELF, 350, MS_LEADER, MZ_HUMAN), MR_SLEEP, MR_SLEEP, */
/*         M1_HUMANOID | M1_SEE_INVIS | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_ELF | M2_HUMAN | M2_PNAME | M2_PEACEFUL | M2_STRONG */
/*           | M2_MALE | M2_COLLECT | M2_MAGIC, */
/*         M3_CLOSE | M3_INFRAVISION | M3_INFRAVISIBLE, HI_LORD), */
/*     MON("Elwing", S_HUMAN, */
/*         LVL(20, 12, 0, 50, -20), (G_NOGEN | G_UNIQ), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 8), */
/*           NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_ELF, 350, MS_LEADER, MZ_HUMAN), MR_SLEEP, MR_SLEEP, */
/*         M1_HUMANOID | M1_SEE_INVIS | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_ELF | M2_HUMAN | M2_PNAME | M2_PEACEFUL | M2_STRONG */
/*           | M2_FEMALE | M2_COLLECT | M2_MAGIC, */
/*         M3_CLOSE | M3_INFRAVISION | M3_INFRAVISIBLE, HI_LORD), */
/* #endif */
/*     MON("Hippocrates", S_HUMAN, LVL(20, 12, 0, 40, 0), (G_NOGEN | G_UNIQ), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_LEADER, MZ_HUMAN), MR_POISON, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_PNAME | M2_PEACEFUL | M2_STRONG | M2_MALE */
/*             | M2_COLLECT | M2_MAGIC, */
/*         M3_CLOSE | M3_INFRAVISIBLE, HI_LORD), */
/*     MON("King Arthur", S_HUMAN, LVL(20, 12, 0, 40, 20), (G_NOGEN | G_UNIQ), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_LEADER, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_PNAME | M2_PEACEFUL | M2_STRONG | M2_MALE */
/*             | M2_COLLECT | M2_MAGIC, */
/*         M3_CLOSE | M3_INFRAVISIBLE, HI_LORD), */
/*     MON("Grand Master", S_HUMAN, LVL(25, 12, 0, 70, 0), (G_NOGEN | G_UNIQ), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 4, 10), ATTK(AT_KICK, AD_PHYS, 2, 8), */
/*           ATTK(AT_MAGC, AD_CLRC, 2, 8), ATTK(AT_MAGC, AD_CLRC, 2, 8), NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_LEADER, MZ_HUMAN), */
/*         MR_FIRE | MR_ELEC | MR_SLEEP | MR_POISON, 0, */
/*         M1_HUMANOID | M1_SEE_INVIS | M1_HERBIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_MALE | M2_NASTY */
/*             | M2_MAGIC, */
/*         M3_CLOSE | M3_INFRAVISIBLE, CLR_BLACK), */
/*     MON("Arch Priest", S_HUMAN, LVL(25, 12, 7, 70, 0), (G_NOGEN | G_UNIQ), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 4, 10), ATTK(AT_KICK, AD_PHYS, 2, 8), */
/*           ATTK(AT_MAGC, AD_CLRC, 2, 8), ATTK(AT_MAGC, AD_CLRC, 2, 8), NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_LEADER, MZ_HUMAN), */
/*         MR_FIRE | MR_ELEC | MR_SLEEP | MR_POISON, 0, */
/*         M1_HUMANOID | M1_SEE_INVIS | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_MALE | M2_COLLECT */
/*             | M2_MAGIC, */
/*         M3_CLOSE | M3_INFRAVISIBLE, CLR_WHITE), */
/*     MON("Orion", S_HUMAN, LVL(20, 12, 0, 30, 0), (G_NOGEN | G_UNIQ), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(2200, 700, MS_LEADER, MZ_HUGE), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE | M1_SEE_INVIS | M1_SWIM | M1_AMPHIBIOUS, */
/*         M2_NOPOLY | M2_HUMAN | M2_PNAME | M2_PEACEFUL | M2_STRONG | M2_MALE */
/*             | M2_COLLECT | M2_MAGIC, */
/*         M3_CLOSE | M3_INFRAVISION | M3_INFRAVISIBLE, HI_LORD), */
/*     #<{(| Note: Master of Thieves is also the Tourist's nemesis. */
/*      |)}># */
/*     MON("Master of Thieves", S_HUMAN, LVL(20, 12, 0, 30, -20), */
/*         (G_NOGEN | G_UNIQ), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 6), ATTK(AT_WEAP, AD_PHYS, 2, 6), */
/*           ATTK(AT_CLAW, AD_SAMU, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_LEADER, MZ_HUMAN), MR_STONE, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_MALE | M2_GREEDY */
/*             | M2_JEWELS | M2_COLLECT | M2_MAGIC, */
/*         M3_CLOSE | M3_INFRAVISIBLE, HI_LORD), */
/*     MON("Lord Sato", S_HUMAN, LVL(20, 12, 0, 30, 20), (G_NOGEN | G_UNIQ), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 8), ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_LEADER, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_PNAME | M2_PEACEFUL | M2_STRONG | M2_MALE */
/*             | M2_COLLECT | M2_MAGIC, */
/*         M3_CLOSE | M3_INFRAVISIBLE, HI_LORD), */
/*     MON("Twoflower", S_HUMAN, LVL(20, 12, 10, 20, 0), (G_NOGEN | G_UNIQ), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_LEADER, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_PNAME | M2_PEACEFUL | M2_STRONG | M2_MALE */
/*             | M2_COLLECT | M2_MAGIC, */
/*         M3_CLOSE | M3_INFRAVISIBLE, HI_DOMESTIC), */
/*     MON("Norn", S_HUMAN, LVL(20, 12, 0, 80, 0), (G_NOGEN | G_UNIQ), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 8), ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1800, 550, MS_LEADER, MZ_HUGE), MR_COLD, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_FEMALE */
/*             | M2_COLLECT | M2_MAGIC, */
/*         M3_CLOSE | M3_INFRAVISIBLE, HI_LORD), */
/*     MON("Neferet the Green", S_HUMAN, LVL(20, 12, 0, 60, 0), */
/*         (G_NOGEN | G_UNIQ), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_MAGC, AD_SPEL, 2, 8), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_LEADER, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_FEMALE | M2_PNAME | M2_PEACEFUL | M2_STRONG */
/*             | M2_COLLECT | M2_MAGIC, */
/*         M3_CLOSE | M3_INFRAVISIBLE, CLR_GREEN), */
/*     #<{(| */
/*      * quest nemeses */
/*      |)}># */
/*     MON("Minion of Huhetotl", S_DEMON, LVL(16, 12, -2, 75, -14), */
/*         (G_NOCORPSE | G_NOGEN | G_UNIQ), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 8, 4), ATTK(AT_WEAP, AD_PHYS, 4, 6), */
/*           ATTK(AT_MAGC, AD_SPEL, 0, 0), ATTK(AT_CLAW, AD_SAMU, 2, 6), NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_NEMESIS, MZ_LARGE), */
/*         MR_FIRE | MR_POISON | MR_STONE, 0, M1_FLY | M1_SEE_INVIS | M1_POIS, */
/*         M2_NOPOLY | M2_DEMON | M2_STALK | M2_HOSTILE | M2_STRONG | M2_NASTY */
/*             | M2_COLLECT, */
/*         M3_WANTSARTI | M3_WAITFORU | M3_INFRAVISION | M3_INFRAVISIBLE, */
/*         CLR_RED), */
/*     MON("Thoth Amon", S_HUMAN, LVL(16, 12, 0, 10, -14), */
/*         (G_NOGEN | G_UNIQ | G_NOCORPSE), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_MAGC, AD_SPEL, 0, 0), */
/*           ATTK(AT_MAGC, AD_SPEL, 0, 0), ATTK(AT_CLAW, AD_SAMU, 1, 4), NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_NEMESIS, MZ_HUMAN), MR_POISON | MR_STONE, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_PNAME | M2_STRONG | M2_MALE | M2_STALK */
/*             | M2_HOSTILE | M2_NASTY | M2_COLLECT | M2_MAGIC, */
/*         M3_WANTSARTI | M3_WAITFORU | M3_INFRAVISIBLE, HI_LORD), */
/*     #<{(| Multi-headed, possessing the breath attacks of all the other dragons */
/*      * (selected at random when attacking). */
/*      |)}># */
/*     MON("Chromatic Dragon", S_DRAGON, LVL(16, 12, 0, 30, -14), */
/*         (G_NOGEN | G_UNIQ), */
/*         A(ATTK(AT_BREA, AD_RBRE, 6, 6), ATTK(AT_MAGC, AD_SPEL, 0, 0), */
/*           ATTK(AT_CLAW, AD_SAMU, 2, 8), ATTK(AT_BITE, AD_PHYS, 4, 8), */
/*           ATTK(AT_BITE, AD_PHYS, 4, 8), ATTK(AT_STNG, AD_PHYS, 1, 6)), */
/*         SIZ(WT_DRAGON, 1700, MS_NEMESIS, MZ_GIGANTIC), */
/*         MR_FIRE | MR_COLD | MR_SLEEP | MR_DISINT | MR_ELEC | MR_POISON */
/*             | MR_ACID | MR_STONE, */
/*         MR_FIRE | MR_COLD | MR_SLEEP | MR_DISINT | MR_ELEC | MR_POISON */
/*             | MR_STONE, */
/*         M1_THICK_HIDE | M1_NOHANDS | M1_CARNIVORE | M1_SEE_INVIS | M1_POIS, */
/*         M2_NOPOLY | M2_HOSTILE | M2_FEMALE | M2_STALK | M2_STRONG | M2_NASTY */
/*             | M2_GREEDY | M2_JEWELS | M2_MAGIC, */
/*         M3_WANTSARTI | M3_WAITFORU | M3_INFRAVISIBLE, HI_LORD), */
/* #if 0 #<{(| OBSOLETE |)}># */
/*     MON("Goblin King", S_ORC, */
/*         LVL(15, 12, 10, 0, -15), (G_NOGEN | G_UNIQ), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 6), ATTK(AT_WEAP, AD_PHYS, 2, 6), */
/*           ATTK(AT_CLAW, AD_SAMU, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(750, 350, MS_NEMESIS, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_ORC | M2_HOSTILE | M2_STRONG | M2_STALK | M2_NASTY */
/*           | M2_MALE | M2_GREEDY | M2_JEWELS | M2_COLLECT | M2_MAGIC, */
/*         M3_WANTSARTI | M3_WAITFORU | M3_INFRAVISION | M3_INFRAVISIBLE, */
/*         HI_LORD), */
/* #endif */
/*     MON("Cyclops", S_GIANT, LVL(18, 12, 0, 0, -15), (G_NOGEN | G_UNIQ), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 4, 8), ATTK(AT_WEAP, AD_PHYS, 4, 8), */
/*           ATTK(AT_CLAW, AD_SAMU, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(1900, 700, MS_NEMESIS, MZ_HUGE), MR_STONE, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_GIANT | M2_STRONG | M2_ROCKTHROW | M2_STALK */
/*             | M2_HOSTILE | M2_NASTY | M2_MALE | M2_JEWELS | M2_COLLECT, */
/*         M3_WANTSARTI | M3_WAITFORU | M3_INFRAVISION | M3_INFRAVISIBLE, */
/*         CLR_GRAY), */
/*     MON("Ixoth", S_DRAGON, LVL(15, 12, -1, 20, -14), (G_NOGEN | G_UNIQ), */
/*         A(ATTK(AT_BREA, AD_FIRE, 8, 6), ATTK(AT_BITE, AD_PHYS, 4, 8), */
/*           ATTK(AT_MAGC, AD_SPEL, 0, 0), ATTK(AT_CLAW, AD_PHYS, 2, 4), */
/*           ATTK(AT_CLAW, AD_SAMU, 2, 4), NO_ATTK), */
/*         SIZ(WT_DRAGON, 1600, MS_NEMESIS, MZ_GIGANTIC), MR_FIRE | MR_STONE, */
/*         MR_FIRE, */
/*         M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_CARNIVORE | M1_SEE_INVIS, */
/*         M2_NOPOLY | M2_MALE | M2_PNAME | M2_HOSTILE | M2_STRONG | M2_NASTY */
/*             | M2_STALK | M2_GREEDY | M2_JEWELS | M2_MAGIC, */
/*         M3_WANTSARTI | M3_WAITFORU | M3_INFRAVISIBLE, CLR_RED), */
/*     MON("Master Kaen", S_HUMAN, LVL(25, 12, -10, 10, -20), (G_NOGEN | G_UNIQ), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 16, 2), ATTK(AT_CLAW, AD_PHYS, 16, 2), */
/*           ATTK(AT_MAGC, AD_CLRC, 0, 0), ATTK(AT_CLAW, AD_SAMU, 1, 4), NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_NEMESIS, MZ_HUMAN), MR_POISON | MR_STONE, */
/*         MR_POISON, M1_HUMANOID | M1_HERBIVORE | M1_SEE_INVIS, */
/*         M2_NOPOLY | M2_HUMAN | M2_MALE | M2_PNAME | M2_HOSTILE | M2_STRONG */
/*             | M2_NASTY | M2_STALK | M2_COLLECT | M2_MAGIC, */
/*         M3_WANTSARTI | M3_WAITFORU | M3_INFRAVISIBLE, HI_LORD), */
/*     MON("Nalzok", S_DEMON, LVL(16, 12, -2, 85, -127), */
/*         (G_NOGEN | G_UNIQ | G_NOCORPSE), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 8, 4), ATTK(AT_WEAP, AD_PHYS, 4, 6), */
/*           ATTK(AT_MAGC, AD_SPEL, 0, 0), ATTK(AT_CLAW, AD_SAMU, 2, 6), NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_NEMESIS, MZ_LARGE), */
/*         MR_FIRE | MR_POISON | MR_STONE, 0, M1_FLY | M1_SEE_INVIS | M1_POIS, */
/*         M2_NOPOLY | M2_DEMON | M2_MALE | M2_PNAME | M2_HOSTILE | M2_STRONG */
/*             | M2_STALK | M2_NASTY | M2_COLLECT, */
/*         M3_WANTSARTI | M3_WAITFORU | M3_INFRAVISION | M3_INFRAVISIBLE, */
/*         CLR_RED), */
/*     MON("Scorpius", S_SPIDER, LVL(15, 12, 10, 0, -15), (G_NOGEN | G_UNIQ), */
/*         A(ATTK(AT_CLAW, AD_PHYS, 2, 6), ATTK(AT_CLAW, AD_SAMU, 2, 6), */
/*           ATTK(AT_STNG, AD_DISE, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(750, 350, MS_NEMESIS, MZ_HUMAN), MR_POISON | MR_STONE, MR_POISON, */
/*         M1_ANIMAL | M1_NOHANDS | M1_OVIPAROUS | M1_POIS | M1_CARNIVORE, */
/*         M2_NOPOLY | M2_MALE | M2_PNAME | M2_HOSTILE | M2_STRONG | M2_STALK */
/*             | M2_NASTY | M2_COLLECT | M2_MAGIC, */
/*         M3_WANTSARTI | M3_WAITFORU, HI_LORD), */
/*     MON("Master Assassin", S_HUMAN, LVL(15, 12, 0, 30, 18), */
/*         (G_NOGEN | G_UNIQ), */
/*         A(ATTK(AT_WEAP, AD_DRST, 2, 6), ATTK(AT_WEAP, AD_PHYS, 2, 8), */
/*           ATTK(AT_CLAW, AD_SAMU, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_NEMESIS, MZ_HUMAN), MR_STONE, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_MALE | M2_HOSTILE | M2_STALK */
/*             | M2_NASTY | M2_COLLECT | M2_MAGIC, */
/*         M3_WANTSARTI | M3_WAITFORU | M3_INFRAVISIBLE, HI_LORD), */
/*     #<{(| A renegade daimyo who led a 13 year civil war against the shogun */
/*      * of his time. */
/*      |)}># */
/*     MON("Ashikaga Takauji", S_HUMAN, LVL(15, 12, 0, 40, -13), */
/*         (G_NOGEN | G_UNIQ | G_NOCORPSE), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 6), ATTK(AT_WEAP, AD_PHYS, 2, 6), */
/*           ATTK(AT_CLAW, AD_SAMU, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_NEMESIS, MZ_HUMAN), MR_STONE, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_PNAME | M2_HOSTILE | M2_STRONG | M2_STALK */
/*             | M2_NASTY | M2_MALE | M2_COLLECT | M2_MAGIC, */
/*         M3_WANTSARTI | M3_WAITFORU | M3_INFRAVISIBLE, HI_LORD), */
/*     #<{(| */
/*      * Note: the Master of Thieves was defined above. */
/*      |)}># */
/*     MON("Lord Surtur", S_GIANT, LVL(15, 12, 2, 50, 12), (G_NOGEN | G_UNIQ), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 10), ATTK(AT_WEAP, AD_PHYS, 2, 10), */
/*           ATTK(AT_CLAW, AD_SAMU, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(2250, 850, MS_NEMESIS, MZ_HUGE), MR_FIRE | MR_STONE, MR_FIRE, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_GIANT | M2_MALE | M2_PNAME | M2_HOSTILE | M2_STALK */
/*             | M2_STRONG | M2_NASTY | M2_ROCKTHROW | M2_JEWELS | M2_COLLECT, */
/*         M3_WANTSARTI | M3_WAITFORU | M3_INFRAVISION | M3_INFRAVISIBLE, */
/*         HI_LORD), */
/*     MON("Dark One", S_HUMAN, LVL(15, 12, 0, 80, -10), */
/*         (G_NOGEN | G_UNIQ | G_NOCORPSE), */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_WEAP, AD_PHYS, 1, 6), */
/*           ATTK(AT_CLAW, AD_SAMU, 1, 4), ATTK(AT_MAGC, AD_SPEL, 0, 0), NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_NEMESIS, MZ_HUMAN), MR_STONE, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_HOSTILE | M2_STALK | M2_NASTY */
/*             | M2_COLLECT | M2_MAGIC, */
/*         M3_WANTSARTI | M3_WAITFORU | M3_INFRAVISIBLE, CLR_BLACK), */
/*     #<{(| */
/*      * quest "guardians" */
/*      |)}># */
/*     MON("student", S_HUMAN, LVL(5, 12, 10, 10, 3), G_NOGEN, */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_GUARDIAN, MZ_HUMAN), 0, 0, */
/*         M1_TUNNEL | M1_NEEDPICK | M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_COLLECT, */
/*         M3_INFRAVISIBLE, HI_DOMESTIC), */
/*     MON("chieftain", S_HUMAN, LVL(5, 12, 10, 10, 0), G_NOGEN, */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_GUARDIAN, MZ_HUMAN), MR_POISON, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_COLLECT, */
/*         M3_INFRAVISIBLE, HI_DOMESTIC), */
/*     MON("neanderthal", S_HUMAN, LVL(5, 12, 10, 10, 1), G_NOGEN, */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_GUARDIAN, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_COLLECT, */
/*         M3_INFRAVISIBLE, HI_DOMESTIC), */
/* #if 0 #<{(| OBSOLETE |)}># */
/*     MON("High-elf", S_HUMAN, */
/*         LVL(5, 12, 10, 10, -7), G_NOGEN, */
/*         A(ATTK(AT_WEAP, AD_PHYS, 2, 4), ATTK(AT_MAGC, AD_CLRC, 0, 0), */
/*           NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_ELF, 350, MS_GUARDIAN, MZ_HUMAN), MR_SLEEP, MR_SLEEP, */
/*         M1_HUMANOID | M1_SEE_INVIS | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_ELF | M2_PEACEFUL | M2_COLLECT, */
/*         M3_INFRAVISION | M3_INFRAVISIBLE, HI_DOMESTIC), */
/* #endif */
/*     MON("attendant", S_HUMAN, LVL(5, 12, 10, 10, 3), G_NOGEN, */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_GUARDIAN, MZ_HUMAN), MR_POISON, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_COLLECT, */
/*         M3_INFRAVISIBLE, HI_DOMESTIC), */
/*     MON("page", S_HUMAN, LVL(5, 12, 10, 10, 3), G_NOGEN, */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_GUARDIAN, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_COLLECT, */
/*         M3_INFRAVISIBLE, HI_DOMESTIC), */
/*     MON("abbot", S_HUMAN, LVL(5, 12, 10, 20, 0), G_NOGEN, */
/*         A(ATTK(AT_CLAW, AD_PHYS, 8, 2), ATTK(AT_KICK, AD_STUN, 3, 2), */
/*           ATTK(AT_MAGC, AD_CLRC, 0, 0), NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_GUARDIAN, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_HERBIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_COLLECT, */
/*         M3_INFRAVISIBLE, HI_DOMESTIC), */
/*     MON("acolyte", S_HUMAN, LVL(5, 12, 10, 20, 0), G_NOGEN, */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_MAGC, AD_CLRC, 0, 0), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_GUARDIAN, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_COLLECT, */
/*         M3_INFRAVISIBLE, HI_DOMESTIC), */
/*     MON("hunter", S_HUMAN, LVL(5, 12, 10, 10, -7), G_NOGEN, */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, */
/*           NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_GUARDIAN, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_SEE_INVIS | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_COLLECT, */
/*         M3_INFRAVISION | M3_INFRAVISIBLE, HI_DOMESTIC), */
/*     MON("thug", S_HUMAN, LVL(5, 12, 10, 10, -3), G_NOGEN, */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_GUARDIAN, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, M2_NOPOLY | M2_HUMAN | M2_PEACEFUL */
/*                                        | M2_STRONG | M2_GREEDY | M2_COLLECT, */
/*         M3_INFRAVISIBLE, HI_DOMESTIC), */
/*     MON("ninja", S_HUMAN, LVL(5, 12, 10, 10, 3), G_NOGEN, */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 8), ATTK(AT_WEAP, AD_PHYS, 1, 8), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_HOSTILE | M2_STRONG | M2_COLLECT, */
/*         M3_INFRAVISIBLE, HI_DOMESTIC), */
/*     MON("roshi", S_HUMAN, LVL(5, 12, 10, 10, 3), G_NOGEN, */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 8), ATTK(AT_WEAP, AD_PHYS, 1, 8), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_GUARDIAN, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, */
/*         M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_COLLECT, */
/*         M3_INFRAVISIBLE, HI_DOMESTIC), */
/*     MON("guide", S_HUMAN, LVL(5, 12, 10, 20, 0), G_NOGEN, */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_MAGC, AD_SPEL, 0, 0), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_GUARDIAN, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, M2_NOPOLY | M2_HUMAN | M2_PEACEFUL */
/*                                        | M2_STRONG | M2_COLLECT | M2_MAGIC, */
/*         M3_INFRAVISIBLE, HI_DOMESTIC), */
/*     MON("warrior", S_HUMAN, LVL(5, 12, 10, 10, -1), G_NOGEN, */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 8), ATTK(AT_WEAP, AD_PHYS, 1, 8), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_GUARDIAN, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, M2_NOPOLY | M2_HUMAN | M2_PEACEFUL */
/*                                        | M2_STRONG | M2_COLLECT | M2_FEMALE, */
/*         M3_INFRAVISIBLE, HI_DOMESTIC), */
/*     MON("apprentice", S_HUMAN, LVL(5, 12, 10, 30, 0), G_NOGEN, */
/*         A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_MAGC, AD_SPEL, 0, 0), NO_ATTK, */
/*           NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(WT_HUMAN, 400, MS_GUARDIAN, MZ_HUMAN), 0, 0, */
/*         M1_HUMANOID | M1_OMNIVORE, M2_NOPOLY | M2_HUMAN | M2_PEACEFUL */
/*                                        | M2_STRONG | M2_COLLECT | M2_MAGIC, */
/*         M3_INFRAVISIBLE, HI_DOMESTIC), */
/*     #<{(| */
/*      * array terminator */
/*      |)}># */
/*     MON("", 0, LVL(0, 0, 0, 0, 0), (0), */
/*         A(NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK), */
/*         SIZ(0, 0, 0, 0), 0, 0, 0L, 0L, 0, 0) */
/* }; */
/* #endif #<{(| !SPLITMON_1 |)}># */
#ifndef SPLITMON_2
NEARDATA struct permonst mons[] = {
    /*
     * ants
     */
    MON("giant ant", "{{giant ant}}", S_ANT, LVL(2, 18, 3, 0, 0), (G_GENO | G_SGROUP | 3),
        A(ATTK(AT_BITE, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(10, 10, MS_SILENT, MZ_TINY), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_OVIPAROUS | M1_CARNIVORE, M2_HOSTILE, 0,
        CLR_BROWN),
    MON("killer bee", "{{killer bee}}", S_ANT, LVL(1, 18, -1, 0, 0), (G_GENO | G_LGROUP | 2),
        A(ATTK(AT_STNG, AD_DRST, 1, 3), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1, 5, MS_BUZZ, MZ_TINY), MR_POISON, MR_POISON,
        M1_ANIMAL | M1_FLY | M1_NOHANDS | M1_POIS, M2_HOSTILE | M2_FEMALE, 0,
        CLR_YELLOW),
    MON("soldier ant", "{{soldier ant}}", S_ANT, LVL(3, 18, 3, 0, 0), (G_GENO | G_SGROUP | 2),
        A(ATTK(AT_BITE, AD_PHYS, 2, 4), ATTK(AT_STNG, AD_DRST, 3, 4), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(20, 5, MS_SILENT, MZ_TINY), MR_POISON, MR_POISON,
        M1_ANIMAL | M1_NOHANDS | M1_OVIPAROUS | M1_POIS | M1_CARNIVORE,
        M2_HOSTILE, 0, CLR_BLUE),
    MON("fire ant", "{{fire ant}}", S_ANT, LVL(3, 18, 3, 10, 0), (G_GENO | G_SGROUP | 1),
        A(ATTK(AT_BITE, AD_PHYS, 2, 4), ATTK(AT_BITE, AD_FIRE, 2, 4), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(30, 10, MS_SILENT, MZ_TINY), MR_FIRE, MR_FIRE,
        M1_ANIMAL | M1_NOHANDS | M1_OVIPAROUS | M1_CARNIVORE, M2_HOSTILE,
        M3_INFRAVISIBLE, CLR_RED),
    MON("giant beetle", "{{giant beetle}}", S_ANT, LVL(5, 6, 4, 0, 0), (G_GENO | 3),
        A(ATTK(AT_BITE, AD_PHYS, 3, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(10, 10, MS_SILENT, MZ_LARGE), MR_POISON, MR_POISON,
        M1_ANIMAL | M1_NOHANDS | M1_POIS | M1_CARNIVORE, M2_HOSTILE, 0,
        CLR_BLACK),
    MON("queen bee", "{{queen bee}}", S_ANT, LVL(9, 24, -4, 0, 0), (G_GENO | G_NOGEN),
        A(ATTK(AT_STNG, AD_DRST, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1, 5, MS_BUZZ, MZ_TINY), MR_POISON, MR_POISON,
        M1_ANIMAL | M1_FLY | M1_NOHANDS | M1_OVIPAROUS | M1_POIS,
        M2_HOSTILE | M2_FEMALE | M2_PRINCE, 0, HI_LORD),
    /*
     * blobs
     */
    MON("acid blob", "{{acid blob}}", S_BLOB, LVL(1, 3, 8, 0, 0), (G_GENO | 2),
        A(ATTK(AT_NONE, AD_ACID, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(30, 10, MS_SILENT, MZ_TINY),
        MR_SLEEP | MR_POISON | MR_ACID | MR_STONE, MR_STONE,
        M1_BREATHLESS | M1_AMORPHOUS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD
            | M1_MINDLESS | M1_ACID,
        M2_WANDER | M2_NEUTER, 0, CLR_GREEN),
    MON("quivering blob", "{{quivering blob}}", S_BLOB, LVL(5, 1, 8, 0, 0), (G_GENO | 2),
        A(ATTK(AT_TUCH, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(200, 100, MS_SILENT, MZ_SMALL), MR_SLEEP | MR_POISON, MR_POISON,
        M1_NOEYES | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS,
        M2_WANDER | M2_HOSTILE | M2_NEUTER, 0, CLR_WHITE),
    MON("gelatinous cube", "{{gelatinous cube}}", S_BLOB, LVL(6, 6, 8, 0, 0), (G_GENO | 2),
        A(ATTK(AT_TUCH, AD_PLYS, 2, 4), ATTK(AT_NONE, AD_PLYS, 1, 4), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(600, 150, MS_SILENT, MZ_LARGE),
        MR_FIRE | MR_COLD | MR_ELEC | MR_SLEEP | MR_POISON | MR_ACID
            | MR_STONE,
        MR_FIRE | MR_COLD | MR_ELEC | MR_SLEEP,
        M1_NOEYES | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS | M1_OMNIVORE
            | M1_ACID,
        M2_WANDER | M2_HOSTILE | M2_NEUTER, 0, CLR_CYAN),
    /*
     * cockatrice
     */
    MON("chickatrice", "{{chickatrice}}", S_COCKATRICE, LVL(4, 4, 8, 30, 0),
        (G_GENO | G_SGROUP | 1),
        A(ATTK(AT_BITE, AD_PHYS, 1, 2), ATTK(AT_TUCH, AD_STON, 0, 0),
          ATTK(AT_NONE, AD_STON, 0, 0), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(10, 10, MS_HISS, MZ_TINY), MR_POISON | MR_STONE,
        MR_POISON | MR_STONE, M1_ANIMAL | M1_NOHANDS | M1_OMNIVORE,
        M2_HOSTILE, M3_INFRAVISIBLE, CLR_BROWN),
    MON("cockatrice", "{{cockatrice}}", S_COCKATRICE, LVL(5, 6, 6, 30, 0), (G_GENO | 5),
        A(ATTK(AT_BITE, AD_PHYS, 1, 3), ATTK(AT_TUCH, AD_STON, 0, 0),
          ATTK(AT_NONE, AD_STON, 0, 0), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(30, 30, MS_HISS, MZ_SMALL), MR_POISON | MR_STONE,
        MR_POISON | MR_STONE,
        M1_ANIMAL | M1_NOHANDS | M1_OMNIVORE | M1_OVIPAROUS, M2_HOSTILE,
        M3_INFRAVISIBLE, CLR_YELLOW),
    MON("pyrolisk", "{{pyrolisk}}", S_COCKATRICE, LVL(6, 6, 6, 30, 0), (G_GENO | 1),
        A(ATTK(AT_GAZE, AD_FIRE, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(30, 30, MS_HISS, MZ_SMALL), MR_POISON | MR_FIRE,
        MR_POISON | MR_FIRE,
        M1_ANIMAL | M1_NOHANDS | M1_OMNIVORE | M1_OVIPAROUS, M2_HOSTILE,
        M3_INFRAVISIBLE, CLR_RED),
    /*
     * dogs & other canines
     */
    MON("jackal", "{{jackal}}", S_DOG, LVL(0, 12, 7, 0, 0), (G_GENO | G_SGROUP | 3),
        A(ATTK(AT_BITE, AD_PHYS, 1, 2), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(300, 250, MS_BARK, MZ_SMALL), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, M3_INFRAVISIBLE,
        CLR_BROWN),
    MON("fox", "{{fox}}", S_DOG, LVL(0, 15, 7, 0, 0), (G_GENO | 1),
        A(ATTK(AT_BITE, AD_PHYS, 1, 3), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(300, 250, MS_BARK, MZ_SMALL), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, M3_INFRAVISIBLE,
        CLR_RED),
    MON("coyote", "{{coyote}}", S_DOG, LVL(1, 12, 7, 0, 0), (G_GENO | G_SGROUP | 1),
        A(ATTK(AT_BITE, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(300, 250, MS_BARK, MZ_SMALL), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, M3_INFRAVISIBLE,
        CLR_BROWN),
    MON("werejackal", "{{werejackal}}", S_DOG, LVL(2, 12, 7, 10, -7), (G_NOGEN | G_NOCORPSE),
        A(ATTK(AT_BITE, AD_WERE, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(300, 250, MS_BARK, MZ_SMALL), MR_POISON, 0,
        M1_NOHANDS | M1_POIS | M1_REGEN | M1_CARNIVORE,
        M2_NOPOLY | M2_WERE | M2_HOSTILE, M3_INFRAVISIBLE, CLR_BROWN),
    MON("little dog", "{{little dog}}", S_DOG, LVL(2, 18, 6, 0, 0), (G_GENO | 1),
        A(ATTK(AT_BITE, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(150, 150, MS_BARK, MZ_SMALL), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_DOMESTIC, M3_INFRAVISIBLE,
        HI_DOMESTIC),
    MON("dingo", "{{dingo}}", S_DOG, LVL(4, 16, 5, 0, 0), (G_GENO | 1),
        A(ATTK(AT_BITE, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(400, 200, MS_BARK, MZ_MEDIUM), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, M3_INFRAVISIBLE,
        CLR_YELLOW),
    MON("dog", "{{dog}}", S_DOG, LVL(4, 16, 5, 0, 0), (G_GENO | 1),
        A(ATTK(AT_BITE, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(400, 200, MS_BARK, MZ_MEDIUM), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_DOMESTIC, M3_INFRAVISIBLE,
        HI_DOMESTIC),
    MON("large dog", "{{large dog}}", S_DOG, LVL(6, 15, 4, 0, 0), (G_GENO | 1),
        A(ATTK(AT_BITE, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(800, 250, MS_BARK, MZ_MEDIUM), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_STRONG | M2_DOMESTIC,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("wolf", "{{wolf}}", S_DOG, LVL(5, 12, 4, 0, 0), (G_GENO | G_SGROUP | 2),
        A(ATTK(AT_BITE, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(500, 250, MS_BARK, MZ_MEDIUM), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, M3_INFRAVISIBLE,
        CLR_BROWN),
    MON("werewolf", "{{werewolf}}", S_DOG, LVL(5, 12, 4, 20, -7), (G_NOGEN | G_NOCORPSE),
        A(ATTK(AT_BITE, AD_WERE, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(500, 250, MS_BARK, MZ_MEDIUM), MR_POISON, 0,
        M1_NOHANDS | M1_POIS | M1_REGEN | M1_CARNIVORE,
        M2_NOPOLY | M2_WERE | M2_HOSTILE, M3_INFRAVISIBLE, CLR_BROWN),
    MON("winter wolf cub", "{{winter wolf cub}}", S_DOG, LVL(5, 12, 4, 0, -5),
        (G_NOHELL | G_GENO | G_SGROUP | 2),
        A(ATTK(AT_BITE, AD_PHYS, 1, 8), ATTK(AT_BREA, AD_COLD, 1, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(250, 200, MS_BARK, MZ_SMALL), MR_COLD, MR_COLD,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, 0, CLR_CYAN),
    MON("warg", "{{warg}}", S_DOG, LVL(7, 12, 4, 0, -5), (G_GENO | G_SGROUP | 2),
        A(ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(850, 350, MS_BARK, MZ_MEDIUM), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, M3_INFRAVISIBLE,
        CLR_BROWN),
    MON("winter wolf", "{{winter wolf}}", S_DOG, LVL(7, 12, 4, 20, 0), (G_NOHELL | G_GENO | 1),
        A(ATTK(AT_BITE, AD_PHYS, 2, 6), ATTK(AT_BREA, AD_COLD, 2, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(700, 300, MS_BARK, MZ_LARGE), MR_COLD, MR_COLD,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE | M2_STRONG, 0,
        CLR_CYAN),
    MON("hell hound pup", "{{hell hound pup}}", S_DOG, LVL(7, 12, 4, 20, -5),
        (G_HELL | G_GENO | G_SGROUP | 1),
        A(ATTK(AT_BITE, AD_PHYS, 2, 6), ATTK(AT_BREA, AD_FIRE, 2, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(200, 200, MS_BARK, MZ_SMALL), MR_FIRE, MR_FIRE,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, M3_INFRAVISIBLE,
        CLR_RED),
    MON("hell hound", "{{hell hound}}", S_DOG, LVL(12, 14, 2, 20, 0), (G_HELL | G_GENO | 1),
        A(ATTK(AT_BITE, AD_PHYS, 3, 6), ATTK(AT_BREA, AD_FIRE, 3, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(600, 300, MS_BARK, MZ_MEDIUM), MR_FIRE, MR_FIRE,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE | M2_STRONG,
        M3_INFRAVISIBLE, CLR_RED),
#ifdef CHARON
    MON("Cerberus", "{{Cerberus}}", S_DOG, LVL(12, 10, 2, 20, -7),
        (G_NOGEN | G_UNIQ | G_HELL),
        A(ATTK(AT_BITE, AD_PHYS, 3, 6), ATTK(AT_BITE, AD_PHYS, 3, 6),
          ATTK(AT_BITE, AD_PHYS, 3, 6), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1000, 350, MS_BARK, MZ_LARGE), MR_FIRE, MR_FIRE,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE,
        M2_NOPOLY | M2_HOSTILE | M2_STRONG | M2_PNAME | M2_MALE,
        M3_INFRAVISIBLE, CLR_RED),
#endif
    /*
     * eyes
     */
    MON("gas spore", "{{gas spore}}", S_EYE, LVL(1, 3, 10, 0, 0), (G_NOCORPSE | G_GENO | 1),
        A(ATTK(AT_BOOM, AD_PHYS, 4, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(10, 10, MS_SILENT, MZ_SMALL), 0, 0,
        M1_FLY | M1_BREATHLESS | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS,
        M2_HOSTILE | M2_NEUTER, 0, CLR_GRAY),
    MON("floating eye", "{{floating eye}}", S_EYE, LVL(2, 1, 9, 10, 0), (G_GENO | 5),
        A(ATTK(AT_NONE, AD_PLYS, 0, 70), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(10, 10, MS_SILENT, MZ_SMALL), 0, 0,
        M1_FLY | M1_AMPHIBIOUS | M1_NOLIMBS | M1_NOHEAD | M1_NOTAKE,
        M2_HOSTILE | M2_NEUTER, M3_INFRAVISIBLE, CLR_BLUE),
    MON("freezing sphere", "{{freezing sphere}}", S_EYE, LVL(6, 13, 4, 0, 0),
        (G_NOCORPSE | G_NOHELL | G_GENO | 2),
        A(ATTK(AT_EXPL, AD_COLD, 4, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(10, 10, MS_SILENT, MZ_SMALL), MR_COLD, MR_COLD,
        M1_FLY | M1_BREATHLESS | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS
            | M1_NOTAKE,
        M2_HOSTILE | M2_NEUTER, M3_INFRAVISIBLE, CLR_WHITE),
    MON("flaming sphere", "{{flaming sphere}}", S_EYE, LVL(6, 13, 4, 0, 0),
        (G_NOCORPSE | G_GENO | 2), A(ATTK(AT_EXPL, AD_FIRE, 4, 6), NO_ATTK,
                                     NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(10, 10, MS_SILENT, MZ_SMALL), MR_FIRE, MR_FIRE,
        M1_FLY | M1_BREATHLESS | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS
            | M1_NOTAKE,
        M2_HOSTILE | M2_NEUTER, M3_INFRAVISIBLE, CLR_RED),
    MON("shocking sphere", "{{shocking sphere}}", S_EYE, LVL(6, 13, 4, 0, 0),
        (G_NOCORPSE | G_GENO | 2), A(ATTK(AT_EXPL, AD_ELEC, 4, 6), NO_ATTK,
                                     NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(10, 10, MS_SILENT, MZ_SMALL), MR_ELEC, MR_ELEC,
        M1_FLY | M1_BREATHLESS | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS
            | M1_NOTAKE,
        M2_HOSTILE | M2_NEUTER, M3_INFRAVISIBLE, HI_ZAP),
#if 0 /* not yet implemented */
    MON("beholder", "{{beholder}}", S_EYE,
        LVL(6, 3, 4, 0, -10), (G_GENO | 2),
        A(ATTK(AT_GAZE, AD_SLOW, 0, 0), ATTK(AT_GAZE, AD_SLEE, 2,25),
          ATTK(AT_GAZE, AD_DISN, 0, 0), ATTK(AT_GAZE, AD_STON, 0, 0),
          ATTK(AT_GAZE, AD_CNCL, 2, 4), ATTK(AT_BITE, AD_PHYS, 2, 4)),
        SIZ(10, 10, MS_SILENT, MZ_SMALL), MR_COLD, 0,
        M1_FLY | M1_BREATHLESS | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS,
        M2_NOPOLY | M2_HOSTILE | M2_NEUTER, M3_INFRAVISIBLE, CLR_BROWN),
#endif
    /*
     * felines
     */
    MON("kitten", "{{kitten}}", S_FELINE, LVL(2, 18, 6, 0, 0), (G_GENO | 1),
        A(ATTK(AT_BITE, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(150, 150, MS_MEW, MZ_SMALL), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_WANDER | M2_DOMESTIC,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("housecat", "{{housecat}}", S_FELINE, LVL(4, 16, 5, 0, 0), (G_GENO | 1),
        A(ATTK(AT_BITE, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(200, 200, MS_MEW, MZ_SMALL), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_DOMESTIC, M3_INFRAVISIBLE,
        HI_DOMESTIC),
    MON("jaguar", "{{jaguar}}", S_FELINE, LVL(4, 15, 6, 0, 0), (G_GENO | 2),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4),
          ATTK(AT_BITE, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(600, 300, MS_GROWL, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, M3_INFRAVISIBLE,
        CLR_BROWN),
    MON("lynx", "{{lynx}}", S_FELINE, LVL(5, 15, 6, 0, 0), (G_GENO | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4),
          ATTK(AT_BITE, AD_PHYS, 1, 10), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(600, 300, MS_GROWL, MZ_SMALL), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, M3_INFRAVISIBLE,
        CLR_CYAN),
    MON("panther", "{{panther}}", S_FELINE, LVL(5, 15, 6, 0, 0), (G_GENO | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 6), ATTK(AT_CLAW, AD_PHYS, 1, 6),
          ATTK(AT_BITE, AD_PHYS, 1, 10), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(600, 300, MS_GROWL, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, M3_INFRAVISIBLE,
        CLR_BLACK),
    MON("large cat", "{{large cat}}", S_FELINE, LVL(6, 15, 4, 0, 0), (G_GENO | 1),
        A(ATTK(AT_BITE, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(250, 250, MS_MEW, MZ_SMALL), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_STRONG | M2_DOMESTIC,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("tiger", "{{tiger}}", S_FELINE, LVL(6, 12, 6, 0, 0), (G_GENO | 2),
        A(ATTK(AT_CLAW, AD_PHYS, 2, 4), ATTK(AT_CLAW, AD_PHYS, 2, 4),
          ATTK(AT_BITE, AD_PHYS, 1, 10), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(600, 300, MS_GROWL, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, M3_INFRAVISIBLE,
        CLR_YELLOW),
    /*
     * gremlins and gargoyles
     */
    MON("gremlin", "{{gremlin}}", S_GREMLIN, LVL(5, 12, 2, 25, -9), (G_GENO | 2),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 6), ATTK(AT_CLAW, AD_PHYS, 1, 6),
          ATTK(AT_BITE, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_CURS, 0, 0), NO_ATTK,
          NO_ATTK),
        SIZ(100, 20, MS_LAUGH, MZ_SMALL), MR_POISON, MR_POISON,
        M1_SWIM | M1_HUMANOID | M1_POIS, M2_STALK, M3_INFRAVISIBLE,
        CLR_GREEN),
    MON("gargoyle", "{{gargoyle}}", S_GREMLIN, LVL(6, 10, -4, 0, -9), (G_GENO | 2),
        A(ATTK(AT_CLAW, AD_PHYS, 2, 6), ATTK(AT_CLAW, AD_PHYS, 2, 6),
          ATTK(AT_BITE, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1000, 200, MS_GRUNT, MZ_HUMAN), MR_STONE, MR_STONE,
        M1_HUMANOID | M1_THICK_HIDE | M1_BREATHLESS, M2_HOSTILE | M2_STRONG,
        0, CLR_BROWN),
    MON("winged gargoyle", "{{winged gargoyle}}", S_GREMLIN, LVL(9, 15, -2, 0, -12), (G_GENO | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 3, 6), ATTK(AT_CLAW, AD_PHYS, 3, 6),
          ATTK(AT_BITE, AD_PHYS, 3, 4), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1200, 300, MS_GRUNT, MZ_HUMAN), MR_STONE, MR_STONE,
        M1_FLY | M1_HUMANOID | M1_THICK_HIDE | M1_BREATHLESS | M1_OVIPAROUS,
        M2_LORD | M2_HOSTILE | M2_STRONG | M2_MAGIC, 0, HI_LORD),
    /*
     * humanoids
     */
    MON("hobbit", "{{hobbit}}", S_HUMANOID, LVL(1, 9, 10, 0, 6), (G_GENO | 2),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(500, 200, MS_HUMANOID, MZ_SMALL), 0, 0, M1_HUMANOID | M1_OMNIVORE,
        M2_COLLECT, M3_INFRAVISIBLE | M3_INFRAVISION, CLR_GREEN),
    MON("dwarf", "{{dwarf}}", S_HUMANOID, LVL(2, 6, 10, 10, 4), (G_GENO | 3),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(900, 300, MS_HUMANOID, MZ_HUMAN), 0, 0,
        M1_TUNNEL | M1_NEEDPICK | M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_DWARF | M2_STRONG | M2_GREEDY | M2_JEWELS | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED),
    MON("bugbear", "{{bugbear}}", S_HUMANOID, LVL(3, 9, 5, 0, -6), (G_GENO | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1250, 250, MS_GROWL, MZ_LARGE), 0, 0, M1_HUMANOID | M1_OMNIVORE,
        M2_STRONG | M2_COLLECT, M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BROWN),
    MON("dwarf lord", "{{dwarf lord}}", S_HUMANOID, LVL(4, 6, 10, 10, 5), (G_GENO | 2),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 4), ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(900, 300, MS_HUMANOID, MZ_HUMAN), 0, 0,
        M1_TUNNEL | M1_NEEDPICK | M1_HUMANOID | M1_OMNIVORE,
        M2_DWARF | M2_STRONG | M2_LORD | M2_MALE | M2_GREEDY | M2_JEWELS
            | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BLUE),
    MON("dwarf king", "{{dwarf king}}", S_HUMANOID, LVL(6, 6, 10, 20, 6), (G_GENO | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 6), ATTK(AT_WEAP, AD_PHYS, 2, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(900, 300, MS_HUMANOID, MZ_HUMAN), 0, 0,
        M1_TUNNEL | M1_NEEDPICK | M1_HUMANOID | M1_OMNIVORE,
        M2_DWARF | M2_STRONG | M2_PRINCE | M2_MALE | M2_GREEDY | M2_JEWELS
            | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, HI_LORD),
    MON("mind flayer", "{{mind flayer}}", S_HUMANOID, LVL(9, 12, 5, 90, -8), (G_GENO | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 4), ATTK(AT_TENT, AD_DRIN, 2, 1),
          ATTK(AT_TENT, AD_DRIN, 2, 1), ATTK(AT_TENT, AD_DRIN, 2, 1), NO_ATTK,
          NO_ATTK),
        SIZ(1450, 400, MS_HISS, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_FLY | M1_SEE_INVIS | M1_OMNIVORE,
        M2_HOSTILE | M2_NASTY | M2_GREEDY | M2_JEWELS | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_MAGENTA),
    MON("master mind flayer", "{{master mind flayer}}", S_HUMANOID, LVL(13, 12, 0, 90, -8),
        (G_GENO | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 8), ATTK(AT_TENT, AD_DRIN, 2, 1),
          ATTK(AT_TENT, AD_DRIN, 2, 1), ATTK(AT_TENT, AD_DRIN, 2, 1),
          ATTK(AT_TENT, AD_DRIN, 2, 1), ATTK(AT_TENT, AD_DRIN, 2, 1)),
        SIZ(1450, 400, MS_HISS, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_FLY | M1_SEE_INVIS | M1_OMNIVORE,
        M2_HOSTILE | M2_NASTY | M2_GREEDY | M2_JEWELS | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_MAGENTA),
    /*
     * imps & other minor demons/devils
     */
    MON("manes", "{{manes}}", S_IMP, LVL(1, 3, 7, 0, -7),
        (G_GENO | G_LGROUP | G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 3), ATTK(AT_CLAW, AD_PHYS, 1, 3),
          ATTK(AT_BITE, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(100, 100, MS_SILENT, MZ_SMALL), MR_SLEEP | MR_POISON, 0, M1_POIS,
        M2_HOSTILE | M2_STALK, M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED),
    MON("homunculus", "{{homunculus}}", S_IMP, LVL(2, 12, 6, 10, -7), (G_GENO | 2),
        A(ATTK(AT_BITE, AD_SLEE, 1, 3), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(60, 100, MS_SILENT, MZ_TINY), MR_SLEEP | MR_POISON,
        MR_SLEEP | MR_POISON, M1_FLY | M1_POIS, M2_STALK,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_GREEN),
    MON("imp", "{{imp}}", S_IMP, LVL(3, 12, 2, 20, -7), (G_GENO | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(20, 10, MS_CUSS, MZ_TINY), 0, 0, M1_REGEN, M2_WANDER | M2_STALK,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED),
    MON("lemure", "{{lemure}}", S_IMP, LVL(3, 3, 7, 0, -7),
        (G_HELL | G_GENO | G_LGROUP | G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 3), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(150, 100, MS_SILENT, MZ_MEDIUM), MR_SLEEP | MR_POISON, MR_SLEEP,
        M1_POIS | M1_REGEN, M2_HOSTILE | M2_WANDER | M2_STALK | M2_NEUTER,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BROWN),
    MON("quasit", "{{quasit}}", S_IMP, LVL(3, 15, 2, 20, -7), (G_GENO | 2),
        A(ATTK(AT_CLAW, AD_DRDX, 1, 2), ATTK(AT_CLAW, AD_DRDX, 1, 2),
          ATTK(AT_BITE, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(200, 200, MS_SILENT, MZ_SMALL), MR_POISON, MR_POISON, M1_REGEN,
        M2_STALK, M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BLUE),
    MON("tengu", "{{tengu}}", S_IMP, LVL(6, 13, 5, 30, 7), (G_GENO | 3),
        A(ATTK(AT_BITE, AD_PHYS, 1, 7), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(300, 200, MS_SQAWK, MZ_SMALL), MR_POISON, MR_POISON,
        M1_TPORT | M1_TPORT_CNTRL, M2_STALK, M3_INFRAVISIBLE | M3_INFRAVISION,
        CLR_CYAN),
    /*
     * jellies
     */
    MON("blue jelly", "{{blue jelly}}", S_JELLY, LVL(4, 0, 8, 10, 0), (G_GENO | 2),
        A(ATTK(AT_NONE, AD_COLD, 0, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(50, 20, MS_SILENT, MZ_MEDIUM), MR_COLD | MR_POISON,
        MR_COLD | MR_POISON,
        M1_BREATHLESS | M1_AMORPHOUS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD
            | M1_MINDLESS | M1_NOTAKE,
        M2_HOSTILE | M2_NEUTER, 0, CLR_BLUE),
    MON("spotted jelly", "{{spotted jelly}}", S_JELLY, LVL(5, 0, 8, 10, 0), (G_GENO | 1),
        A(ATTK(AT_NONE, AD_ACID, 0, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(50, 20, MS_SILENT, MZ_MEDIUM), MR_ACID | MR_STONE, 0,
        M1_BREATHLESS | M1_AMORPHOUS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD
            | M1_MINDLESS | M1_ACID | M1_NOTAKE,
        M2_HOSTILE | M2_NEUTER, 0, CLR_GREEN),
    MON("ochre jelly", "{{ochre jelly}}", S_JELLY, LVL(6, 3, 8, 20, 0), (G_GENO | 2),
        A(ATTK(AT_ENGL, AD_ACID, 3, 6), ATTK(AT_NONE, AD_ACID, 3, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(50, 20, MS_SILENT, MZ_MEDIUM), MR_ACID | MR_STONE, 0,
        M1_BREATHLESS | M1_AMORPHOUS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD
            | M1_MINDLESS | M1_ACID | M1_NOTAKE,
        M2_HOSTILE | M2_NEUTER, 0, CLR_BROWN),
    /*
     * kobolds
     */
    MON("kobold", "{{kobold}}", S_KOBOLD, LVL(0, 6, 10, 0, -2), (G_GENO | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(400, 100, MS_ORC, MZ_SMALL), MR_POISON, 0,
        M1_HUMANOID | M1_POIS | M1_OMNIVORE, M2_HOSTILE | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BROWN),
    MON("large kobold", "{{large kobold}}", S_KOBOLD, LVL(1, 6, 10, 0, -3), (G_GENO | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(450, 150, MS_ORC, MZ_SMALL), MR_POISON, 0,
        M1_HUMANOID | M1_POIS | M1_OMNIVORE, M2_HOSTILE | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED),
    MON("kobold lord", "{{kobold lord}}", S_KOBOLD, LVL(2, 6, 10, 0, -4), (G_GENO | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(500, 200, MS_ORC, MZ_SMALL), MR_POISON, 0,
        M1_HUMANOID | M1_POIS | M1_OMNIVORE,
        M2_HOSTILE | M2_LORD | M2_MALE | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, HI_LORD),
    MON("kobold shaman", "{{kobold shaman}}", S_KOBOLD, LVL(2, 6, 6, 10, -4), (G_GENO | 1),
        A(ATTK(AT_MAGC, AD_SPEL, 0, 0), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(450, 150, MS_ORC, MZ_SMALL), MR_POISON, 0,
        M1_HUMANOID | M1_POIS | M1_OMNIVORE, M2_HOSTILE | M2_MAGIC,
        M3_INFRAVISIBLE | M3_INFRAVISION, HI_ZAP),
    /*
     * leprechauns
     */
    MON("leprechaun", "{{leprechaun}}", S_LEPRECHAUN, LVL(5, 15, 8, 20, 0), (G_GENO | 4),
        A(ATTK(AT_CLAW, AD_SGLD, 1, 2), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(60, 30, MS_LAUGH, MZ_TINY), 0, 0, M1_HUMANOID | M1_TPORT,
        M2_HOSTILE | M2_GREEDY, M3_INFRAVISIBLE, CLR_GREEN),
    /*
     * mimics
     */
    MON("small mimic", "{{small mimic}}", S_MIMIC, LVL(7, 3, 7, 0, 0), (G_GENO | 2),
        A(ATTK(AT_CLAW, AD_PHYS, 3, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(300, 200, MS_SILENT, MZ_MEDIUM), MR_ACID, 0,
        M1_BREATHLESS | M1_AMORPHOUS | M1_HIDE | M1_ANIMAL | M1_NOEYES
            | M1_NOHEAD | M1_NOLIMBS | M1_THICK_HIDE | M1_CARNIVORE,
        M2_HOSTILE, 0, CLR_BROWN),
    MON("large mimic", "{{large mimic}}", S_MIMIC, LVL(8, 3, 7, 10, 0), (G_GENO | 1),
        A(ATTK(AT_CLAW, AD_STCK, 3, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(600, 400, MS_SILENT, MZ_LARGE), MR_ACID, 0,
        M1_CLING | M1_BREATHLESS | M1_AMORPHOUS | M1_HIDE | M1_ANIMAL
            | M1_NOEYES | M1_NOHEAD | M1_NOLIMBS | M1_THICK_HIDE
            | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG, 0, CLR_RED),
    MON("giant mimic", "{{giant mimic}}", S_MIMIC, LVL(9, 3, 7, 20, 0), (G_GENO | 1),
        A(ATTK(AT_CLAW, AD_STCK, 3, 6), ATTK(AT_CLAW, AD_STCK, 3, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(800, 500, MS_SILENT, MZ_LARGE), MR_ACID, 0,
        M1_CLING | M1_BREATHLESS | M1_AMORPHOUS | M1_HIDE | M1_ANIMAL
            | M1_NOEYES | M1_NOHEAD | M1_NOLIMBS | M1_THICK_HIDE
            | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG, 0, HI_LORD),
    /*
     * nymphs
     */
    MON("wood nymph", "{{wood nymph}}", S_NYMPH, LVL(3, 12, 9, 20, 0), (G_GENO | 2),
        A(ATTK(AT_CLAW, AD_SITM, 0, 0), ATTK(AT_CLAW, AD_SEDU, 0, 0), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(600, 300, MS_SEDUCE, MZ_HUMAN), 0, 0, M1_HUMANOID | M1_TPORT,
        M2_HOSTILE | M2_FEMALE | M2_COLLECT, M3_INFRAVISIBLE, CLR_GREEN),
    MON("water nymph", "{{water nymph}}", S_NYMPH, LVL(3, 12, 9, 20, 0), (G_GENO | 2),
        A(ATTK(AT_CLAW, AD_SITM, 0, 0), ATTK(AT_CLAW, AD_SEDU, 0, 0), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(600, 300, MS_SEDUCE, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_TPORT | M1_SWIM, M2_HOSTILE | M2_FEMALE | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_BLUE),
    MON("mountain nymph", "{{mountain nymph}}", S_NYMPH, LVL(3, 12, 9, 20, 0), (G_GENO | 2),
        A(ATTK(AT_CLAW, AD_SITM, 0, 0), ATTK(AT_CLAW, AD_SEDU, 0, 0), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(600, 300, MS_SEDUCE, MZ_HUMAN), 0, 0, M1_HUMANOID | M1_TPORT,
        M2_HOSTILE | M2_FEMALE | M2_COLLECT, M3_INFRAVISIBLE, CLR_BROWN),
    /*
     * orcs
     */
    MON("goblin", "{{goblin}}", S_ORC, LVL(0, 6, 10, 0, -3), (G_GENO | 2),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(400, 100, MS_ORC, MZ_SMALL), 0, 0, M1_HUMANOID | M1_OMNIVORE,
        M2_ORC | M2_COLLECT, M3_INFRAVISIBLE | M3_INFRAVISION, CLR_GRAY),
    MON("hobgoblin", "{{hobgoblin}}", S_ORC, LVL(1, 9, 10, 0, -4), (G_GENO | 2),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1000, 200, MS_ORC, MZ_HUMAN), 0, 0, M1_HUMANOID | M1_OMNIVORE,
        M2_ORC | M2_STRONG | M2_COLLECT, M3_INFRAVISIBLE | M3_INFRAVISION,
        CLR_BROWN),
    /* plain "orc" for zombie corpses only; not created at random
     */
    MON("orc", "{{orc}}", S_ORC, LVL(1, 9, 10, 0, -3), (G_GENO | G_NOGEN | G_LGROUP),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(850, 150, MS_ORC, MZ_HUMAN), 0, 0, M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_ORC | M2_STRONG | M2_GREEDY | M2_JEWELS | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED),
    MON("hill orc", "{{hill orc}}", S_ORC, LVL(2, 9, 10, 0, -4), (G_GENO | G_LGROUP | 2),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1000, 200, MS_ORC, MZ_HUMAN), 0, 0, M1_HUMANOID | M1_OMNIVORE,
        M2_ORC | M2_STRONG | M2_GREEDY | M2_JEWELS | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_YELLOW),
    MON("Mordor orc", "{{Mordor orc}}", S_ORC, LVL(3, 5, 10, 0, -5), (G_GENO | G_LGROUP | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1200, 200, MS_ORC, MZ_HUMAN), 0, 0, M1_HUMANOID | M1_OMNIVORE,
        M2_ORC | M2_STRONG | M2_GREEDY | M2_JEWELS | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BLUE),
    MON("Uruk-hai", "{{Uruk-hai}}", S_ORC, LVL(3, 7, 10, 0, -4), (G_GENO | G_LGROUP | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1300, 300, MS_ORC, MZ_HUMAN), 0, 0, M1_HUMANOID | M1_OMNIVORE,
        M2_ORC | M2_STRONG | M2_GREEDY | M2_JEWELS | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BLACK),
    MON("orc shaman", "{{orc shaman}}", S_ORC, LVL(3, 9, 5, 10, -5), (G_GENO | 1),
        A(ATTK(AT_MAGC, AD_SPEL, 0, 0), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1000, 300, MS_ORC, MZ_HUMAN), 0, 0, M1_HUMANOID | M1_OMNIVORE,
        M2_ORC | M2_STRONG | M2_GREEDY | M2_JEWELS | M2_MAGIC,
        M3_INFRAVISIBLE | M3_INFRAVISION, HI_ZAP),
    MON("orc-captain", "{{orc-captain}}", S_ORC, LVL(5, 5, 10, 0, -5), (G_GENO | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 4), ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1350, 350, MS_ORC, MZ_HUMAN), 0, 0, M1_HUMANOID | M1_OMNIVORE,
        M2_ORC | M2_STRONG | M2_GREEDY | M2_JEWELS | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, HI_LORD),
    /*
     * piercers
     */
    MON("rock piercer", "{{rock piercer}}", S_PIERCER, LVL(3, 1, 3, 0, 0), (G_GENO | 4),
        A(ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(200, 200, MS_SILENT, MZ_SMALL), 0, 0,
        M1_CLING | M1_HIDE | M1_ANIMAL | M1_NOEYES | M1_NOLIMBS | M1_CARNIVORE
            | M1_NOTAKE,
        M2_HOSTILE, 0, CLR_GRAY),
    MON("iron piercer", "{{iron piercer}}", S_PIERCER, LVL(5, 1, 0, 0, 0), (G_GENO | 2),
        A(ATTK(AT_BITE, AD_PHYS, 3, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(400, 300, MS_SILENT, MZ_MEDIUM), 0, 0,
        M1_CLING | M1_HIDE | M1_ANIMAL | M1_NOEYES | M1_NOLIMBS | M1_CARNIVORE
            | M1_NOTAKE,
        M2_HOSTILE, 0, CLR_CYAN),
    MON("glass piercer", "{{glass piercer}}", S_PIERCER, LVL(7, 1, 0, 0, 0), (G_GENO | 1),
        A(ATTK(AT_BITE, AD_PHYS, 4, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(400, 300, MS_SILENT, MZ_MEDIUM), MR_ACID, 0,
        M1_CLING | M1_HIDE | M1_ANIMAL | M1_NOEYES | M1_NOLIMBS | M1_CARNIVORE
            | M1_NOTAKE,
        M2_HOSTILE, 0, CLR_WHITE),
    /*
     * quadrupeds
     */
    MON("rothe", "{{rothe}}", S_QUADRUPED, LVL(2, 9, 7, 0, 0), (G_GENO | G_SGROUP | 4),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 3), ATTK(AT_BITE, AD_PHYS, 1, 3),
          ATTK(AT_BITE, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(400, 100, MS_SILENT, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_OMNIVORE, M2_HOSTILE, M3_INFRAVISIBLE,
        CLR_BROWN),
    MON("mumak", "{{mumak}}", S_QUADRUPED, LVL(5, 9, 0, 0, -2), (G_GENO | 1),
        A(ATTK(AT_BUTT, AD_PHYS, 4, 12), ATTK(AT_BITE, AD_PHYS, 2, 6),
          NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(2500, 500, MS_ROAR, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_THICK_HIDE | M1_NOHANDS | M1_HERBIVORE,
        M2_HOSTILE | M2_STRONG, M3_INFRAVISIBLE, CLR_GRAY),
    MON("leocrotta", "{{leocrotta}}", S_QUADRUPED, LVL(6, 18, 4, 10, 0), (G_GENO | 2),
        A(ATTK(AT_CLAW, AD_PHYS, 2, 6), ATTK(AT_BITE, AD_PHYS, 2, 6),
          ATTK(AT_CLAW, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1200, 500, MS_IMITATE, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_OMNIVORE, M2_HOSTILE | M2_STRONG,
        M3_INFRAVISIBLE, CLR_RED),
    MON("wumpus", "{{wumpus}}", S_QUADRUPED, LVL(8, 3, 2, 10, 0), (G_GENO | 1),
        A(ATTK(AT_BITE, AD_PHYS, 3, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(2500, 500, MS_BURBLE, MZ_LARGE), 0, 0,
        M1_CLING | M1_ANIMAL | M1_NOHANDS | M1_OMNIVORE,
        M2_HOSTILE | M2_STRONG, M3_INFRAVISIBLE, CLR_CYAN),
    MON("titanothere", "{{titanothere}}", S_QUADRUPED, LVL(12, 12, 6, 0, 0), (G_GENO | 2),
        A(ATTK(AT_CLAW, AD_PHYS, 2, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(2650, 650, MS_SILENT, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_THICK_HIDE | M1_NOHANDS | M1_HERBIVORE,
        M2_HOSTILE | M2_STRONG, M3_INFRAVISIBLE, CLR_GRAY),
    MON("baluchitherium", "{{baluchitherium}}", S_QUADRUPED, LVL(14, 12, 5, 0, 0), (G_GENO | 2),
        A(ATTK(AT_CLAW, AD_PHYS, 5, 4), ATTK(AT_CLAW, AD_PHYS, 5, 4), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(3800, 800, MS_SILENT, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_THICK_HIDE | M1_NOHANDS | M1_HERBIVORE,
        M2_HOSTILE | M2_STRONG, M3_INFRAVISIBLE, CLR_GRAY),
    MON("mastodon", "{{mastodon}}", S_QUADRUPED, LVL(20, 12, 5, 0, 0), (G_GENO | 1),
        A(ATTK(AT_BUTT, AD_PHYS, 4, 8), ATTK(AT_BUTT, AD_PHYS, 4, 8), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(3800, 800, MS_SILENT, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_THICK_HIDE | M1_NOHANDS | M1_HERBIVORE,
        M2_HOSTILE | M2_STRONG, M3_INFRAVISIBLE, CLR_BLACK),
    /*
     * rodents
     */
    MON("sewer rat", "{{sewer rat}}", S_RODENT, LVL(0, 12, 7, 0, 0), (G_GENO | G_SGROUP | 1),
        A(ATTK(AT_BITE, AD_PHYS, 1, 3), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(20, 12, MS_SQEEK, MZ_TINY), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, M3_INFRAVISIBLE,
        CLR_BROWN),
    MON("giant rat", "{{giant rat}}", S_RODENT, LVL(1, 10, 7, 0, 0), (G_GENO | G_SGROUP | 2),
        A(ATTK(AT_BITE, AD_PHYS, 1, 3), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(30, 30, MS_SQEEK, MZ_TINY), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, M3_INFRAVISIBLE,
        CLR_BROWN),
    MON("rabid rat", "{{rabid rat}}", S_RODENT, LVL(2, 12, 6, 0, 0), (G_GENO | 1),
        A(ATTK(AT_BITE, AD_DRCO, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(30, 5, MS_SQEEK, MZ_TINY), MR_POISON, 0,
        M1_ANIMAL | M1_NOHANDS | M1_POIS | M1_CARNIVORE, M2_HOSTILE,
        M3_INFRAVISIBLE, CLR_BROWN),
    MON("wererat", "{{wererat}}", S_RODENT, LVL(2, 12, 6, 10, -7), (G_NOGEN | G_NOCORPSE),
        A(ATTK(AT_BITE, AD_WERE, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(40, 30, MS_SQEEK, MZ_TINY), MR_POISON, 0,
        M1_NOHANDS | M1_POIS | M1_REGEN | M1_CARNIVORE,
        M2_NOPOLY | M2_WERE | M2_HOSTILE, M3_INFRAVISIBLE, CLR_BROWN),
    MON("rock mole", "{{rock mole}}", S_RODENT, LVL(3, 3, 0, 20, 0), (G_GENO | 2),
        A(ATTK(AT_BITE, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(30, 30, MS_SILENT, MZ_SMALL), 0, 0,
        M1_TUNNEL | M1_ANIMAL | M1_NOHANDS | M1_METALLIVORE,
        M2_HOSTILE | M2_GREEDY | M2_JEWELS | M2_COLLECT, M3_INFRAVISIBLE,
        CLR_GRAY),
    MON("woodchuck", "{{woodchuck}}", S_RODENT, LVL(3, 3, 0, 20, 0), (G_NOGEN | G_GENO),
        A(ATTK(AT_BITE, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(30, 30, MS_SILENT, MZ_SMALL), 0, 0,
        M1_TUNNEL /*LOGGING*/ | M1_ANIMAL | M1_NOHANDS | M1_SWIM
            | M1_HERBIVORE,
        /* In reality, they tunnel instead of cutting lumber.  Oh, well. */
        M2_WANDER | M2_HOSTILE, M3_INFRAVISIBLE, CLR_BROWN),
    /*
     * spiders & scorpions (keep webmaker() in sync if new critters are added)
     */
    MON("cave spider", "{{cave spider}}", S_SPIDER, LVL(1, 12, 3, 0, 0), (G_GENO | G_SGROUP | 2),
        A(ATTK(AT_BITE, AD_PHYS, 1, 2), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(50, 50, MS_SILENT, MZ_TINY), MR_POISON, MR_POISON,
        M1_CONCEAL | M1_ANIMAL | M1_NOHANDS | M1_OVIPAROUS | M1_CARNIVORE,
        M2_HOSTILE, 0, CLR_GRAY),
    MON("centipede", "{{centipede}}", S_SPIDER, LVL(2, 4, 3, 0, 0), (G_GENO | 1),
        A(ATTK(AT_BITE, AD_DRST, 1, 3), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(50, 50, MS_SILENT, MZ_TINY), MR_POISON, MR_POISON,
        M1_CONCEAL | M1_ANIMAL | M1_NOHANDS | M1_OVIPAROUS | M1_CARNIVORE,
        M2_HOSTILE, 0, CLR_YELLOW),
    MON("giant spider", "{{giant spider}}", S_SPIDER, LVL(5, 15, 4, 0, 0), (G_GENO | 1),
        A(ATTK(AT_BITE, AD_DRST, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(100, 100, MS_SILENT, MZ_LARGE), MR_POISON, MR_POISON,
        M1_ANIMAL | M1_NOHANDS | M1_OVIPAROUS | M1_POIS | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG, 0, CLR_MAGENTA),
    MON("scorpion", "{{scorpion}}", S_SPIDER, LVL(5, 15, 3, 0, 0), (G_GENO | 2),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 2), ATTK(AT_CLAW, AD_PHYS, 1, 2),
          ATTK(AT_STNG, AD_DRST, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(50, 100, MS_SILENT, MZ_SMALL), MR_POISON, MR_POISON,
        M1_CONCEAL | M1_ANIMAL | M1_NOHANDS | M1_OVIPAROUS | M1_POIS
            | M1_CARNIVORE,
        M2_HOSTILE, 0, CLR_RED),
    /*
     * trappers, lurkers, &c
     */
    MON("lurker above", "{{lurker above}}", S_TRAPPER, LVL(10, 3, 3, 0, 0), (G_GENO | 2),
        A(ATTK(AT_ENGL, AD_DGST, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(800, 350, MS_SILENT, MZ_HUGE), 0, 0,
        M1_HIDE | M1_FLY | M1_ANIMAL | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD
            | M1_CARNIVORE,
        M2_HOSTILE | M2_STALK | M2_STRONG, 0, CLR_GRAY),
    MON("trapper", "{{trapper}}", S_TRAPPER, LVL(12, 3, 3, 0, 0), (G_GENO | 2),
        A(ATTK(AT_ENGL, AD_DGST, 1, 10), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(800, 350, MS_SILENT, MZ_HUGE), 0, 0,
        M1_HIDE | M1_ANIMAL | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD
            | M1_CARNIVORE,
        M2_HOSTILE | M2_STALK | M2_STRONG, 0, CLR_GREEN),
    /*
     * unicorns and horses
     */
    MON("pony", "{{pony}}", S_UNICORN, LVL(3, 16, 6, 0, 0), (G_GENO | 2),
        A(ATTK(AT_KICK, AD_PHYS, 1, 6), ATTK(AT_BITE, AD_PHYS, 1, 2), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1300, 250, MS_NEIGH, MZ_MEDIUM), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_HERBIVORE,
        M2_WANDER | M2_STRONG | M2_DOMESTIC, M3_INFRAVISIBLE, CLR_BROWN),
    MON("white unicorn", "{{white unicorn}}", S_UNICORN, LVL(4, 24, 2, 70, 7), (G_GENO | 2),
        A(ATTK(AT_BUTT, AD_PHYS, 1, 12), ATTK(AT_KICK, AD_PHYS, 1, 6),
          NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1300, 300, MS_NEIGH, MZ_LARGE), MR_POISON, MR_POISON,
        M1_NOHANDS | M1_HERBIVORE, M2_WANDER | M2_STRONG | M2_JEWELS,
        M3_INFRAVISIBLE, CLR_WHITE),
    MON("gray unicorn", "{{gray unicorn}}", S_UNICORN, LVL(4, 24, 2, 70, 0), (G_GENO | 1),
        A(ATTK(AT_BUTT, AD_PHYS, 1, 12), ATTK(AT_KICK, AD_PHYS, 1, 6),
          NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1300, 300, MS_NEIGH, MZ_LARGE), MR_POISON, MR_POISON,
        M1_NOHANDS | M1_HERBIVORE, M2_WANDER | M2_STRONG | M2_JEWELS,
        M3_INFRAVISIBLE, CLR_GRAY),
    MON("black unicorn", "{{black unicorn}}", S_UNICORN, LVL(4, 24, 2, 70, -7), (G_GENO | 1),
        A(ATTK(AT_BUTT, AD_PHYS, 1, 12), ATTK(AT_KICK, AD_PHYS, 1, 6),
          NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1300, 300, MS_NEIGH, MZ_LARGE), MR_POISON, MR_POISON,
        M1_NOHANDS | M1_HERBIVORE, M2_WANDER | M2_STRONG | M2_JEWELS,
        M3_INFRAVISIBLE, CLR_BLACK),
    MON("horse", "{{horse}}", S_UNICORN, LVL(5, 20, 5, 0, 0), (G_GENO | 2),
        A(ATTK(AT_KICK, AD_PHYS, 1, 8), ATTK(AT_BITE, AD_PHYS, 1, 3), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1500, 300, MS_NEIGH, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_HERBIVORE,
        M2_WANDER | M2_STRONG | M2_DOMESTIC, M3_INFRAVISIBLE, CLR_BROWN),
    MON("warhorse", "{{warhorse}}", S_UNICORN, LVL(7, 24, 4, 0, 0), (G_GENO | 2),
        A(ATTK(AT_KICK, AD_PHYS, 1, 10), ATTK(AT_BITE, AD_PHYS, 1, 4),
          NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1800, 350, MS_NEIGH, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_HERBIVORE,
        M2_WANDER | M2_STRONG | M2_DOMESTIC, M3_INFRAVISIBLE, CLR_BROWN),
    /*
     * vortices
     */
    MON("fog cloud", "{{fog cloud}}", S_VORTEX, LVL(3, 1, 0, 0, 0), (G_GENO | G_NOCORPSE | 2),
        A(ATTK(AT_ENGL, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(0, 0, MS_SILENT, MZ_HUGE), MR_SLEEP | MR_POISON | MR_STONE, 0,
        M1_FLY | M1_BREATHLESS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD
            | M1_MINDLESS | M1_AMORPHOUS | M1_UNSOLID,
        M2_HOSTILE | M2_NEUTER, 0, CLR_GRAY),
    MON("dust vortex", "{{dust vortex}}", S_VORTEX, LVL(4, 20, 2, 30, 0),
        (G_GENO | G_NOCORPSE | 2), A(ATTK(AT_ENGL, AD_BLND, 2, 8), NO_ATTK,
                                     NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(0, 0, MS_SILENT, MZ_HUGE), MR_SLEEP | MR_POISON | MR_STONE, 0,
        M1_FLY | M1_BREATHLESS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD
            | M1_MINDLESS,
        M2_HOSTILE | M2_NEUTER, 0, CLR_BROWN),
    MON("ice vortex", "{{ice vortex}}", S_VORTEX, LVL(5, 20, 2, 30, 0),
        (G_NOHELL | G_GENO | G_NOCORPSE | 1),
        A(ATTK(AT_ENGL, AD_COLD, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(0, 0, MS_SILENT, MZ_HUGE),
        MR_COLD | MR_SLEEP | MR_POISON | MR_STONE, 0,
        M1_FLY | M1_BREATHLESS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD
            | M1_MINDLESS,
        M2_HOSTILE | M2_NEUTER, M3_INFRAVISIBLE, CLR_CYAN),
    MON("energy vortex", "{{energy vortex}}", S_VORTEX, LVL(6, 20, 2, 30, 0),
        (G_GENO | G_NOCORPSE | 1),
        A(ATTK(AT_ENGL, AD_ELEC, 1, 6), ATTK(AT_ENGL, AD_DREN, 2, 6),
          ATTK(AT_NONE, AD_ELEC, 0, 4), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(0, 0, MS_SILENT, MZ_HUGE),
        MR_ELEC | MR_SLEEP | MR_DISINT | MR_POISON | MR_STONE, 0,
        M1_FLY | M1_BREATHLESS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD
            | M1_MINDLESS | M1_UNSOLID,
        M2_HOSTILE | M2_NEUTER, 0, HI_ZAP),
    MON("steam vortex", "{{steam vortex}}", S_VORTEX, LVL(7, 22, 2, 30, 0),
        (G_HELL | G_GENO | G_NOCORPSE | 2),
        A(ATTK(AT_ENGL, AD_FIRE, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(0, 0, MS_SILENT, MZ_HUGE),
        MR_FIRE | MR_SLEEP | MR_POISON | MR_STONE, 0,
        M1_FLY | M1_BREATHLESS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD
            | M1_MINDLESS | M1_UNSOLID,
        M2_HOSTILE | M2_NEUTER, M3_INFRAVISIBLE, CLR_BLUE),
    MON("fire vortex", "{{fire vortex}}", S_VORTEX, LVL(8, 22, 2, 30, 0),
        (G_HELL | G_GENO | G_NOCORPSE | 1),
        A(ATTK(AT_ENGL, AD_FIRE, 1, 10), ATTK(AT_NONE, AD_FIRE, 0, 4),
          NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(0, 0, MS_SILENT, MZ_HUGE),
        MR_FIRE | MR_SLEEP | MR_POISON | MR_STONE, 0,
        M1_FLY | M1_BREATHLESS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD
            | M1_MINDLESS | M1_UNSOLID,
        M2_HOSTILE | M2_NEUTER, M3_INFRAVISIBLE, CLR_YELLOW),
    /*
     * worms
     */
    MON("baby long worm", "{{baby long worm}}", S_WORM, LVL(5, 3, 5, 0, 0), G_GENO,
        A(ATTK(AT_BITE, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(600, 250, MS_SILENT, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_SLITHY | M1_NOLIMBS | M1_CARNIVORE | M1_NOTAKE,
        M2_HOSTILE, 0, CLR_BROWN),
    MON("baby purple worm", "{{baby purple worm}}", S_WORM, LVL(8, 3, 5, 0, 0), G_GENO,
        A(ATTK(AT_BITE, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(600, 250, MS_SILENT, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_SLITHY | M1_NOLIMBS | M1_CARNIVORE, M2_HOSTILE, 0,
        CLR_MAGENTA),
    MON("long worm", "{{long worm}}", S_WORM, LVL(9, 3, 5, 10, 0), (G_GENO | 2),
        A(ATTK(AT_BITE, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1500, 500, MS_SILENT, MZ_GIGANTIC), 0, 0,
        M1_ANIMAL | M1_SLITHY | M1_NOLIMBS | M1_OVIPAROUS | M1_CARNIVORE
            | M1_NOTAKE,
        M2_HOSTILE | M2_STRONG | M2_NASTY, 0, CLR_BROWN),
    MON("purple worm", "{{purple worm}}", S_WORM, LVL(15, 9, 6, 20, 0), (G_GENO | 2),
        A(ATTK(AT_BITE, AD_PHYS, 2, 8), ATTK(AT_ENGL, AD_DGST, 1, 10),
          NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(2700, 700, MS_SILENT, MZ_GIGANTIC), 0, 0,
        M1_ANIMAL | M1_SLITHY | M1_NOLIMBS | M1_OVIPAROUS | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_NASTY, 0, CLR_MAGENTA),
    /*
     * xan, &c
     */
    MON("grid bug", "{{grid bug}}", S_XAN, LVL(0, 12, 9, 0, 0),
        (G_GENO | G_SGROUP | G_NOCORPSE | 3),
        A(ATTK(AT_BITE, AD_ELEC, 1, 1), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(15, 10, MS_BUZZ, MZ_TINY), MR_ELEC | MR_POISON, 0, M1_ANIMAL,
        M2_HOSTILE, M3_INFRAVISIBLE, CLR_MAGENTA),
    MON("xan", "{{xan}}", S_XAN, LVL(7, 18, -4, 0, 0), (G_GENO | 3),
        A(ATTK(AT_STNG, AD_LEGS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(300, 300, MS_BUZZ, MZ_TINY), MR_POISON, MR_POISON,
        M1_FLY | M1_ANIMAL | M1_NOHANDS | M1_POIS, M2_HOSTILE,
        M3_INFRAVISIBLE, CLR_RED),
    /*
     * lights
     */
    MON("yellow light", "{{yellow light}}", S_LIGHT, LVL(3, 15, 0, 0, 0),
        (G_NOCORPSE | G_GENO | 4), A(ATTK(AT_EXPL, AD_BLND, 10, 20), NO_ATTK,
                                     NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(0, 0, MS_SILENT, MZ_SMALL),
        MR_FIRE | MR_COLD | MR_ELEC | MR_DISINT | MR_SLEEP | MR_POISON
            | MR_ACID | MR_STONE,
        0, M1_FLY | M1_BREATHLESS | M1_AMORPHOUS | M1_NOEYES | M1_NOLIMBS
               | M1_NOHEAD | M1_MINDLESS | M1_UNSOLID | M1_NOTAKE,
        M2_HOSTILE | M2_NEUTER, M3_INFRAVISIBLE, CLR_YELLOW),
    MON("black light", "{{black light}}", S_LIGHT, LVL(5, 15, 0, 0, 0),
        (G_NOCORPSE | G_GENO | 2), A(ATTK(AT_EXPL, AD_HALU, 10, 12), NO_ATTK,
                                     NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(0, 0, MS_SILENT, MZ_SMALL),
        MR_FIRE | MR_COLD | MR_ELEC | MR_DISINT | MR_SLEEP | MR_POISON
            | MR_ACID | MR_STONE,
        0,
        M1_FLY | M1_BREATHLESS | M1_AMORPHOUS | M1_NOEYES | M1_NOLIMBS
            | M1_NOHEAD | M1_MINDLESS | M1_UNSOLID | M1_SEE_INVIS | M1_NOTAKE,
        M2_HOSTILE | M2_NEUTER, 0, CLR_BLACK),
    /*
     * zruty
     */
    MON("zruty", "{{zruty}}", S_ZRUTY, LVL(9, 8, 3, 0, 0), (G_GENO | 2),
        A(ATTK(AT_CLAW, AD_PHYS, 3, 4), ATTK(AT_CLAW, AD_PHYS, 3, 4),
          ATTK(AT_BITE, AD_PHYS, 3, 6), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1200, 600, MS_SILENT, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_HUMANOID | M1_CARNIVORE, M2_HOSTILE | M2_STRONG,
        M3_INFRAVISIBLE, CLR_BROWN),
    /*
     * Angels and other lawful minions
     */
    MON("couatl", "{{couatl}}", S_ANGEL, LVL(8, 10, 5, 30, 7),
        (G_NOHELL | G_SGROUP | G_NOCORPSE | 1),
        A(ATTK(AT_BITE, AD_DRST, 2, 4), ATTK(AT_BITE, AD_PHYS, 1, 3),
          ATTK(AT_HUGS, AD_WRAP, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(900, 400, MS_HISS, MZ_LARGE), MR_POISON, 0,
        M1_FLY | M1_NOHANDS | M1_SLITHY | M1_POIS,
        M2_MINION | M2_STALK | M2_STRONG | M2_NASTY,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_GREEN),
    MON("Aleax", "{{Aleax}}", S_ANGEL, LVL(10, 8, 0, 30, 7), (G_NOHELL | G_NOCORPSE | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_WEAP, AD_PHYS, 1, 6),
          ATTK(AT_KICK, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_IMITATE, MZ_HUMAN),
        MR_COLD | MR_ELEC | MR_SLEEP | MR_POISON, 0,
        M1_HUMANOID | M1_SEE_INVIS,
        M2_MINION | M2_STALK | M2_NASTY | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_YELLOW),
    /* Angels start with the emin extension attached, and usually have
       the isminion flag set; however, non-minion Angels can be tamed
       and will switch to edog (guardian Angel is handled specially and
       always sticks with emin) */
    MON("Angel", "{{Angel}}", S_ANGEL, LVL(14, 10, -4, 55, 12),
        (G_NOHELL | G_NOCORPSE | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_WEAP, AD_PHYS, 1, 6),
          ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_MAGC, AD_MAGM, 2, 6), NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_CUSS, MZ_HUMAN),
        MR_COLD | MR_ELEC | MR_SLEEP | MR_POISON, 0,
        M1_FLY | M1_HUMANOID | M1_SEE_INVIS,
        M2_NOPOLY | M2_MINION | M2_STALK | M2_STRONG | M2_NASTY | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_WHITE),
    MON("ki-rin", "{{ki-rin}}", S_ANGEL, LVL(16, 18, -5, 90, 15),
        (G_NOHELL | G_NOCORPSE | 1),
        A(ATTK(AT_KICK, AD_PHYS, 2, 4), ATTK(AT_KICK, AD_PHYS, 2, 4),
          ATTK(AT_BUTT, AD_PHYS, 3, 6), ATTK(AT_MAGC, AD_SPEL, 2, 6), NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_NEIGH, MZ_LARGE), 0, 0,
        M1_FLY | M1_ANIMAL | M1_NOHANDS | M1_SEE_INVIS,
        M2_NOPOLY | M2_MINION | M2_STALK | M2_STRONG | M2_NASTY | M2_LORD,
        M3_INFRAVISIBLE | M3_INFRAVISION, HI_GOLD),
    MON("Archon", "{{Archon}}", S_ANGEL, LVL(19, 16, -6, 80, 15),
        (G_NOHELL | G_NOCORPSE | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 4), ATTK(AT_WEAP, AD_PHYS, 2, 4),
          ATTK(AT_GAZE, AD_BLND, 2, 6), ATTK(AT_CLAW, AD_PHYS, 1, 8),
          ATTK(AT_MAGC, AD_SPEL, 4, 6), NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_CUSS, MZ_LARGE),
        MR_FIRE | MR_COLD | MR_ELEC | MR_SLEEP | MR_POISON, 0,
        M1_FLY | M1_HUMANOID | M1_SEE_INVIS | M1_REGEN,
        M2_NOPOLY | M2_MINION | M2_STALK | M2_STRONG | M2_NASTY | M2_LORD
            | M2_COLLECT | M2_MAGIC,
        M3_INFRAVISIBLE | M3_INFRAVISION, HI_LORD),
    /*
     * Bats
     */
    MON("bat", "{{bat}}", S_BAT, LVL(0, 22, 8, 0, 0), (G_GENO | G_SGROUP | 1),
        A(ATTK(AT_BITE, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(20, 20, MS_SQEEK, MZ_TINY), 0, 0,
        M1_FLY | M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_WANDER,
        M3_INFRAVISIBLE, CLR_BROWN),
    MON("giant bat", "{{giant bat}}", S_BAT, LVL(2, 22, 7, 0, 0), (G_GENO | 2),
        A(ATTK(AT_BITE, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(30, 30, MS_SQEEK, MZ_SMALL), 0, 0,
        M1_FLY | M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE,
        M2_WANDER | M2_HOSTILE, M3_INFRAVISIBLE, CLR_RED),
    MON("raven", "{{raven}}", S_BAT, LVL(4, 20, 6, 0, 0), (G_GENO | 2),
        A(ATTK(AT_BITE, AD_PHYS, 1, 6), ATTK(AT_CLAW, AD_BLND, 1, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(40, 20, MS_SQAWK, MZ_SMALL), 0, 0,
        M1_FLY | M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE,
        M2_WANDER | M2_HOSTILE, M3_INFRAVISIBLE, CLR_BLACK),
    MON("vampire bat", "{{vampire bat}}", S_BAT, LVL(5, 20, 6, 0, 0), (G_GENO | 2),
        A(ATTK(AT_BITE, AD_PHYS, 1, 6), ATTK(AT_BITE, AD_DRST, 0, 0), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(30, 20, MS_SQEEK, MZ_SMALL), MR_SLEEP | MR_POISON, 0,
        M1_FLY | M1_ANIMAL | M1_NOHANDS | M1_POIS | M1_REGEN | M1_OMNIVORE,
        M2_HOSTILE, M3_INFRAVISIBLE, CLR_BLACK),
    /*
     * Centaurs
     */
    MON("plains centaur", "{{plains centaur}}", S_CENTAUR, LVL(4, 18, 4, 0, 0), (G_GENO | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_KICK, AD_PHYS, 1, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(2500, 500, MS_HUMANOID, MZ_LARGE), 0, 0,
        M1_HUMANOID | M1_OMNIVORE, M2_STRONG | M2_GREEDY | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_BROWN),
    MON("forest centaur", "{{forest centaur}}", S_CENTAUR, LVL(5, 18, 3, 10, -1), (G_GENO | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 8), ATTK(AT_KICK, AD_PHYS, 1, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(2550, 600, MS_HUMANOID, MZ_LARGE), 0, 0,
        M1_HUMANOID | M1_OMNIVORE, M2_STRONG | M2_GREEDY | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_GREEN),
    MON("mountain centaur", "{{mountain centaur}}", S_CENTAUR, LVL(6, 20, 2, 10, -3), (G_GENO | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 10), ATTK(AT_KICK, AD_PHYS, 1, 6),
          ATTK(AT_KICK, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(2550, 500, MS_HUMANOID, MZ_LARGE), 0, 0,
        M1_HUMANOID | M1_OMNIVORE, M2_STRONG | M2_GREEDY | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_CYAN),
    /*
     * Dragons
     */
    /* The order of the dragons is VERY IMPORTANT.  Quite a few
     * pieces of code depend on gray being first and yellow being last.
     * The code also depends on the *order* being the same as that for
     * dragon scale mail and dragon scales in objects.c.  Baby dragons
     * cannot confer intrinsics, to avoid polyself/egg abuse.
     *
     * As reptiles, dragons are cold-blooded and thus aren't seen
     * with infravision.  Red dragons are the exception.
     */
    MON("baby gray dragon", "{{baby gray dragon}}", S_DRAGON, LVL(12, 9, 2, 10, 0), G_GENO,
        A(ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1500, 500, MS_ROAR, MZ_HUGE), 0, 0,
        M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_GREEDY | M2_JEWELS, 0, CLR_GRAY),
    MON("baby silver dragon", "{{baby silver dragon}}", S_DRAGON, LVL(12, 9, 2, 10, 0), G_GENO,
        A(ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1500, 500, MS_ROAR, MZ_HUGE), 0, 0,
        M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_GREEDY | M2_JEWELS, 0, DRAGON_SILVER),
#if 0 /* DEFERRED */
    MON("baby shimmering dragon", "{{baby shimmering dragon}}", S_DRAGON,
        LVL(12, 9, 2, 10, 0), G_GENO,
        A(ATTK(AT_BITE, AD_PHYS, 2, 6),
          NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1500, 500, MS_ROAR, MZ_HUGE), 0, 0,
        M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_GREEDY | M2_JEWELS, 0, CLR_CYAN),
#endif
    MON("baby red dragon", "{{baby red dragon}}", S_DRAGON, LVL(12, 9, 2, 10, 0), G_GENO,
        A(ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1500, 500, MS_ROAR, MZ_HUGE), MR_FIRE, 0,
        M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_GREEDY | M2_JEWELS, M3_INFRAVISIBLE,
        CLR_RED),
    MON("baby white dragon", "{{baby white dragon}}", S_DRAGON, LVL(12, 9, 2, 10, 0), G_GENO,
        A(ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1500, 500, MS_ROAR, MZ_HUGE), MR_COLD, 0,
        M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_GREEDY | M2_JEWELS, 0, CLR_WHITE),
    MON("baby orange dragon", "{{baby orange dragon}}", S_DRAGON, LVL(12, 9, 2, 10, 0), G_GENO,
        A(ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1500, 500, MS_ROAR, MZ_HUGE), MR_SLEEP, 0,
        M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_GREEDY | M2_JEWELS, 0, CLR_ORANGE),
    MON("baby black dragon", "{{baby black dragon}}", S_DRAGON, LVL(12, 9, 2, 10, 0), G_GENO,
        A(ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1500, 500, MS_ROAR, MZ_HUGE), MR_DISINT, 0,
        M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_GREEDY | M2_JEWELS, 0, CLR_BLACK),
    MON("baby blue dragon", "{{baby blue dragon}}", S_DRAGON, LVL(12, 9, 2, 10, 0), G_GENO,
        A(ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1500, 500, MS_ROAR, MZ_HUGE), MR_ELEC, 0,
        M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_GREEDY | M2_JEWELS, 0, CLR_BLUE),
    MON("baby green dragon", "{{baby green dragon}}", S_DRAGON, LVL(12, 9, 2, 10, 0), G_GENO,
        A(ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1500, 500, MS_ROAR, MZ_HUGE), MR_POISON, 0,
        M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_CARNIVORE | M1_POIS,
        M2_HOSTILE | M2_STRONG | M2_GREEDY | M2_JEWELS, 0, CLR_GREEN),
    MON("baby yellow dragon", "{{baby yellow dragon}}", S_DRAGON, LVL(12, 9, 2, 10, 0), G_GENO,
        A(ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1500, 500, MS_ROAR, MZ_HUGE), MR_ACID | MR_STONE, 0,
        M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_CARNIVORE | M1_ACID,
        M2_HOSTILE | M2_STRONG | M2_GREEDY | M2_JEWELS, 0, CLR_YELLOW),
    MON("gray dragon", "{{gray dragon}}", S_DRAGON, LVL(15, 9, -1, 20, 4), (G_GENO | 1),
        A(ATTK(AT_BREA, AD_MAGM, 4, 6), ATTK(AT_BITE, AD_PHYS, 3, 8),
          ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4), NO_ATTK,
          NO_ATTK),
        SIZ(WT_DRAGON, 1500, MS_ROAR, MZ_GIGANTIC), 0, 0,
        M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_SEE_INVIS | M1_OVIPAROUS
            | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_NASTY | M2_GREEDY | M2_JEWELS | M2_MAGIC,
        0, CLR_GRAY),
    MON("silver dragon", "{{silver dragon}}", S_DRAGON, LVL(15, 9, -1, 20, 4), (G_GENO | 1),
        A(ATTK(AT_BREA, AD_COLD, 4, 6), ATTK(AT_BITE, AD_PHYS, 3, 8),
          ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4), NO_ATTK,
          NO_ATTK),
        SIZ(WT_DRAGON, 1500, MS_ROAR, MZ_GIGANTIC), MR_COLD, 0,
        M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_SEE_INVIS | M1_OVIPAROUS
            | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_NASTY | M2_GREEDY | M2_JEWELS | M2_MAGIC,
        0, DRAGON_SILVER),
#if 0 /* DEFERRED */
    MON("shimmering dragon", "{{shimmering dragon}}", S_DRAGON,
        LVL(15, 9, -1, 20, 4), (G_GENO | 1),
        A(ATTK(AT_BREA, AD_MAGM, 4, 6), ATTK(AT_BITE, AD_PHYS, 3, 8),
          ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4),
          NO_ATTK, NO_ATTK),
        SIZ(WT_DRAGON, 1500, MS_ROAR, MZ_GIGANTIC), 0, 0,
        M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_SEE_INVIS | M1_OVIPAROUS
          | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_NASTY | M2_GREEDY | M2_JEWELS | M2_MAGIC,
        0, CLR_CYAN),
#endif
    MON("red dragon", "{{red dragon}}", S_DRAGON, LVL(15, 9, -1, 20, -4), (G_GENO | 1),
        A(ATTK(AT_BREA, AD_FIRE, 6, 6), ATTK(AT_BITE, AD_PHYS, 3, 8),
          ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4), NO_ATTK,
          NO_ATTK),
        SIZ(WT_DRAGON, 1500, MS_ROAR, MZ_GIGANTIC), MR_FIRE, MR_FIRE,
        M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_SEE_INVIS | M1_OVIPAROUS
            | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_NASTY | M2_GREEDY | M2_JEWELS | M2_MAGIC,
        M3_INFRAVISIBLE, CLR_RED),
    MON("white dragon", "{{white dragon}}", S_DRAGON, LVL(15, 9, -1, 20, -5), (G_GENO | 1),
        A(ATTK(AT_BREA, AD_COLD, 4, 6), ATTK(AT_BITE, AD_PHYS, 3, 8),
          ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4), NO_ATTK,
          NO_ATTK),
        SIZ(WT_DRAGON, 1500, MS_ROAR, MZ_GIGANTIC), MR_COLD, MR_COLD,
        M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_SEE_INVIS | M1_OVIPAROUS
            | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_NASTY | M2_GREEDY | M2_JEWELS | M2_MAGIC,
        0, CLR_WHITE),
    MON("orange dragon", "{{orange dragon}}", S_DRAGON, LVL(15, 9, -1, 20, 5), (G_GENO | 1),
        A(ATTK(AT_BREA, AD_SLEE, 4, 25), ATTK(AT_BITE, AD_PHYS, 3, 8),
          ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4), NO_ATTK,
          NO_ATTK),
        SIZ(WT_DRAGON, 1500, MS_ROAR, MZ_GIGANTIC), MR_SLEEP, MR_SLEEP,
        M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_SEE_INVIS | M1_OVIPAROUS
            | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_NASTY | M2_GREEDY | M2_JEWELS | M2_MAGIC,
        0, CLR_ORANGE),
    /* disintegration breath is actually all or nothing, not 1d255 */
    MON("black dragon", "{{black dragon}}", S_DRAGON, LVL(15, 9, -1, 20, -6), (G_GENO | 1),
        A(ATTK(AT_BREA, AD_DISN, 1, 255), ATTK(AT_BITE, AD_PHYS, 3, 8),
          ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4), NO_ATTK,
          NO_ATTK),
        SIZ(WT_DRAGON, 1500, MS_ROAR, MZ_GIGANTIC), MR_DISINT, MR_DISINT,
        M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_SEE_INVIS | M1_OVIPAROUS
            | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_NASTY | M2_GREEDY | M2_JEWELS | M2_MAGIC,
        0, CLR_BLACK),
    MON("blue dragon", "{{blue dragon}}", S_DRAGON, LVL(15, 9, -1, 20, -7), (G_GENO | 1),
        A(ATTK(AT_BREA, AD_ELEC, 4, 6), ATTK(AT_BITE, AD_PHYS, 3, 8),
          ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4), NO_ATTK,
          NO_ATTK),
        SIZ(WT_DRAGON, 1500, MS_ROAR, MZ_GIGANTIC), MR_ELEC, MR_ELEC,
        M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_SEE_INVIS | M1_OVIPAROUS
            | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_NASTY | M2_GREEDY | M2_JEWELS | M2_MAGIC,
        0, CLR_BLUE),
    MON("green dragon", "{{green dragon}}", S_DRAGON, LVL(15, 9, -1, 20, 6), (G_GENO | 1),
        A(ATTK(AT_BREA, AD_DRST, 4, 6), ATTK(AT_BITE, AD_PHYS, 3, 8),
          ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4), NO_ATTK,
          NO_ATTK),
        SIZ(WT_DRAGON, 1500, MS_ROAR, MZ_GIGANTIC), MR_POISON, MR_POISON,
        M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_SEE_INVIS | M1_OVIPAROUS
            | M1_CARNIVORE | M1_POIS,
        M2_HOSTILE | M2_STRONG | M2_NASTY | M2_GREEDY | M2_JEWELS | M2_MAGIC,
        0, CLR_GREEN),
    MON("yellow dragon", "{{yellow dragon}}", S_DRAGON, LVL(15, 9, -1, 20, 7), (G_GENO | 1),
        A(ATTK(AT_BREA, AD_ACID, 4, 6), ATTK(AT_BITE, AD_PHYS, 3, 8),
          ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4), NO_ATTK,
          NO_ATTK),
        SIZ(WT_DRAGON, 1500, MS_ROAR, MZ_GIGANTIC), MR_ACID | MR_STONE,
        MR_STONE, M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_SEE_INVIS
                      | M1_OVIPAROUS | M1_CARNIVORE | M1_ACID,
        M2_HOSTILE | M2_STRONG | M2_NASTY | M2_GREEDY | M2_JEWELS | M2_MAGIC,
        0, CLR_YELLOW),
    /*
     * Elementals
     */
    MON("stalker", "{{stalker}}", S_ELEMENTAL, LVL(8, 12, 3, 0, 0), (G_GENO | 3),
        A(ATTK(AT_CLAW, AD_PHYS, 4, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(900, 400, MS_SILENT, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_FLY | M1_SEE_INVIS,
        M2_WANDER | M2_STALK | M2_HOSTILE | M2_STRONG, M3_INFRAVISION,
        CLR_WHITE),
    MON("air elemental", "{{air elemental}}", S_ELEMENTAL, LVL(8, 36, 2, 30, 0), (G_NOCORPSE | 1),
        A(ATTK(AT_ENGL, AD_PHYS, 1, 10), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(0, 0, MS_SILENT, MZ_HUGE), MR_POISON | MR_STONE, 0,
        M1_NOEYES | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS | M1_BREATHLESS
            | M1_UNSOLID | M1_FLY,
        M2_STRONG | M2_NEUTER, 0, CLR_CYAN),
    MON("fire elemental", "{{fire elemental}}", S_ELEMENTAL, LVL(8, 12, 2, 30, 0), (G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_FIRE, 3, 6), ATTK(AT_NONE, AD_FIRE, 0, 4), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(0, 0, MS_SILENT, MZ_HUGE), MR_FIRE | MR_POISON | MR_STONE, 0,
        M1_NOEYES | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS | M1_BREATHLESS
            | M1_UNSOLID | M1_FLY | M1_NOTAKE,
        M2_STRONG | M2_NEUTER, M3_INFRAVISIBLE, CLR_YELLOW),
    MON("earth elemental", "{{earth elemental}}", S_ELEMENTAL, LVL(8, 6, 2, 30, 0), (G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 4, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(2500, 0, MS_SILENT, MZ_HUGE),
        MR_FIRE | MR_COLD | MR_POISON | MR_STONE, 0,
        M1_NOEYES | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS | M1_BREATHLESS
            | M1_WALLWALK | M1_THICK_HIDE,
        M2_STRONG | M2_NEUTER, 0, CLR_BROWN),
    MON("water elemental", "{{water elemental}}", S_ELEMENTAL, LVL(8, 6, 2, 30, 0), (G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 5, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(2500, 0, MS_SILENT, MZ_HUGE), MR_POISON | MR_STONE, 0,
        M1_NOEYES | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS | M1_BREATHLESS
            | M1_UNSOLID | M1_AMPHIBIOUS | M1_SWIM,
        M2_STRONG | M2_NEUTER, 0, CLR_BLUE),
    /*
     * Fungi
     */
    MON("lichen", "{{lichen}}", S_FUNGUS, LVL(0, 1, 9, 0, 0), (G_GENO | 4),
        A(ATTK(AT_TUCH, AD_STCK, 0, 0), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(20, 200, MS_SILENT, MZ_SMALL), 0, 0,
        M1_BREATHLESS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS
            | M1_NOTAKE,
        M2_HOSTILE | M2_NEUTER, 0, CLR_BRIGHT_GREEN),
    MON("brown mold", "{{brown mold}}", S_FUNGUS, LVL(1, 0, 9, 0, 0), (G_GENO | 1),
        A(ATTK(AT_NONE, AD_COLD, 0, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(50, 30, MS_SILENT, MZ_SMALL), MR_COLD | MR_POISON,
        MR_COLD | MR_POISON, M1_BREATHLESS | M1_NOEYES | M1_NOLIMBS
                                 | M1_NOHEAD | M1_MINDLESS | M1_NOTAKE,
        M2_HOSTILE | M2_NEUTER, 0, CLR_BROWN),
    MON("yellow mold", "{{yellow mold}}", S_FUNGUS, LVL(1, 0, 9, 0, 0), (G_GENO | 2),
        A(ATTK(AT_NONE, AD_STUN, 0, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(50, 30, MS_SILENT, MZ_SMALL), MR_POISON, MR_POISON,
        M1_BREATHLESS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS
            | M1_POIS | M1_NOTAKE,
        M2_HOSTILE | M2_NEUTER, 0, CLR_YELLOW),
    MON("green mold", "{{green mold}}", S_FUNGUS, LVL(1, 0, 9, 0, 0), (G_GENO | 1),
        A(ATTK(AT_NONE, AD_ACID, 0, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(50, 30, MS_SILENT, MZ_SMALL), MR_ACID | MR_STONE, MR_STONE,
        M1_BREATHLESS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS
            | M1_ACID | M1_NOTAKE,
        M2_HOSTILE | M2_NEUTER, 0, CLR_GREEN),
    MON("red mold", "{{red mold}}", S_FUNGUS, LVL(1, 0, 9, 0, 0), (G_GENO | 1),
        A(ATTK(AT_NONE, AD_FIRE, 0, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(50, 30, MS_SILENT, MZ_SMALL), MR_FIRE | MR_POISON,
        MR_FIRE | MR_POISON, M1_BREATHLESS | M1_NOEYES | M1_NOLIMBS
                                 | M1_NOHEAD | M1_MINDLESS | M1_NOTAKE,
        M2_HOSTILE | M2_NEUTER, M3_INFRAVISIBLE, CLR_RED),
    MON("shrieker", "{{shrieker}}", S_FUNGUS, LVL(3, 1, 7, 0, 0), (G_GENO | 1),
        A(NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(100, 100, MS_SHRIEK, MZ_SMALL), MR_POISON, MR_POISON,
        M1_BREATHLESS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS
            | M1_NOTAKE,
        M2_HOSTILE | M2_NEUTER, 0, CLR_MAGENTA),
    MON("violet fungus", "{{violet fungus}}", S_FUNGUS, LVL(3, 1, 7, 0, 0), (G_GENO | 2),
        A(ATTK(AT_TUCH, AD_PHYS, 1, 4), ATTK(AT_TUCH, AD_STCK, 0, 0), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(100, 100, MS_SILENT, MZ_SMALL), MR_POISON, MR_POISON,
        M1_BREATHLESS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS
            | M1_NOTAKE,
        M2_HOSTILE | M2_NEUTER, 0, CLR_MAGENTA),
    /*
     * Gnomes
     */
    MON("gnome", "{{gnome}}", S_GNOME, LVL(1, 6, 10, 4, 0), (G_GENO | G_SGROUP | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(650, 100, MS_ORC, MZ_SMALL), 0, 0, M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_GNOME | M2_COLLECT, M3_INFRAVISIBLE | M3_INFRAVISION,
        CLR_BROWN),
    MON("gnome lord", "{{gnome lord}}", S_GNOME, LVL(3, 8, 10, 4, 0), (G_GENO | 2),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(700, 120, MS_ORC, MZ_SMALL), 0, 0, M1_HUMANOID | M1_OMNIVORE,
        M2_GNOME | M2_LORD | M2_MALE | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BLUE),
    MON("gnomish wizard", "{{gnomish wizard}}", S_GNOME, LVL(3, 10, 4, 10, 0), (G_GENO | 1),
        A(ATTK(AT_MAGC, AD_SPEL, 0, 0), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(700, 120, MS_ORC, MZ_SMALL), 0, 0, M1_HUMANOID | M1_OMNIVORE,
        M2_GNOME | M2_MAGIC, M3_INFRAVISIBLE | M3_INFRAVISION, HI_ZAP),
    MON("gnome king", "{{gnome king}}", S_GNOME, LVL(5, 10, 10, 20, 0), (G_GENO | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(750, 150, MS_ORC, MZ_SMALL), 0, 0, M1_HUMANOID | M1_OMNIVORE,
        M2_GNOME | M2_PRINCE | M2_MALE | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, HI_LORD),
#ifdef SPLITMON_1
};
#endif
#endif /* !SPLITMON_2 */

/* horrible kludge alert:
 * This is a compiler-specific kludge to allow the compilation of monst.o in
 * two pieces, by defining first SPLITMON_1 and then SPLITMON_2. The
 * resulting assembler files (monst1.s and monst2.s) are then run through
 * sed to change local symbols, concatenated together, and assembled to
 * produce monst.o. THIS ONLY WORKS WITH THE ATARI GCC, and should only
 * be done if you don't have enough memory to compile monst.o the "normal"
 * way.  --ERS
 */

#ifndef SPLITMON_1
#ifdef SPLITMON_2
struct permonst _mons2[] = {
#endif
    /*
     * giant Humanoids
     */
    MON("giant", "{{giant}}", S_GIANT, LVL(6, 6, 0, 0, 2), (G_GENO | G_NOGEN | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 10), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(2250, 750, MS_BOAST, MZ_HUGE), 0, 0, M1_HUMANOID | M1_CARNIVORE,
        M2_GIANT | M2_STRONG | M2_ROCKTHROW | M2_NASTY | M2_COLLECT
            | M2_JEWELS,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED),
    MON("stone giant", "{{stone giant}}", S_GIANT, LVL(6, 6, 0, 0, 2), (G_GENO | G_SGROUP | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 10), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(2250, 750, MS_BOAST, MZ_HUGE), 0, 0, M1_HUMANOID | M1_CARNIVORE,
        M2_GIANT | M2_STRONG | M2_ROCKTHROW | M2_NASTY | M2_COLLECT
            | M2_JEWELS,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_GRAY),
    MON("hill giant", "{{hill giant}}", S_GIANT, LVL(8, 10, 6, 0, -2), (G_GENO | G_SGROUP | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(2200, 700, MS_BOAST, MZ_HUGE), 0, 0, M1_HUMANOID | M1_CARNIVORE,
        M2_GIANT | M2_STRONG | M2_ROCKTHROW | M2_NASTY | M2_COLLECT
            | M2_JEWELS,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_CYAN),
    MON("fire giant", "{{fire giant}}", S_GIANT, LVL(9, 12, 4, 5, 2), (G_GENO | G_SGROUP | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 10), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(2250, 750, MS_BOAST, MZ_HUGE), MR_FIRE, MR_FIRE,
        M1_HUMANOID | M1_CARNIVORE, M2_GIANT | M2_STRONG | M2_ROCKTHROW
                                        | M2_NASTY | M2_COLLECT | M2_JEWELS,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_YELLOW),
    MON("frost giant", "{{frost giant}}", S_GIANT, LVL(10, 12, 3, 10, -3),
        (G_NOHELL | G_GENO | G_SGROUP | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 12), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(2250, 750, MS_BOAST, MZ_HUGE), MR_COLD, MR_COLD,
        M1_HUMANOID | M1_CARNIVORE, M2_GIANT | M2_STRONG | M2_ROCKTHROW
                                        | M2_NASTY | M2_COLLECT | M2_JEWELS,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_WHITE),
    MON("ettin", "{{ettin}}", S_GIANT, LVL(10, 12, 3, 0, 0), (G_GENO | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 8), ATTK(AT_WEAP, AD_PHYS, 3, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1700, 500, MS_GRUNT, MZ_HUGE), 0, 0,
        M1_ANIMAL | M1_HUMANOID | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_NASTY | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BROWN),
    MON("storm giant", "{{storm giant}}", S_GIANT, LVL(16, 12, 3, 10, -3),
        (G_GENO | G_SGROUP | 1), A(ATTK(AT_WEAP, AD_PHYS, 2, 12), NO_ATTK,
                                   NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(2250, 750, MS_BOAST, MZ_HUGE), MR_ELEC, MR_ELEC,
        M1_HUMANOID | M1_CARNIVORE, M2_GIANT | M2_STRONG | M2_ROCKTHROW
                                        | M2_NASTY | M2_COLLECT | M2_JEWELS,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BLUE),
    MON("titan", "{{titan}}", S_GIANT, LVL(16, 18, -3, 70, 9), (1),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 8), ATTK(AT_MAGC, AD_SPEL, 0, 0), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(2300, 900, MS_SPELL, MZ_HUGE), 0, 0,
        M1_FLY | M1_HUMANOID | M1_OMNIVORE,
        M2_STRONG | M2_ROCKTHROW | M2_NASTY | M2_COLLECT | M2_MAGIC,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_MAGENTA),
    MON("minotaur", "{{minotaur}}", S_GIANT, LVL(15, 15, 6, 0, 0), (G_GENO | G_NOGEN),
        A(ATTK(AT_CLAW, AD_PHYS, 3, 10), ATTK(AT_CLAW, AD_PHYS, 3, 10),
          ATTK(AT_BUTT, AD_PHYS, 2, 8), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1500, 700, MS_SILENT, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_HUMANOID | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_NASTY, M3_INFRAVISIBLE | M3_INFRAVISION,
        CLR_BROWN),
    /* 'I' is a visual marker for all invisible monsters and must be unused */
    /*
     * Jabberwock
     */
    /* the illustration from _Through_the_Looking_Glass_
       depicts hands as well as wings */
    MON("jabberwock", "{{jabberwock}}", S_JABBERWOCK, LVL(15, 12, -2, 50, 0), (G_GENO | 1),
        A(ATTK(AT_BITE, AD_PHYS, 2, 10), ATTK(AT_BITE, AD_PHYS, 2, 10),
          ATTK(AT_CLAW, AD_PHYS, 2, 10), ATTK(AT_CLAW, AD_PHYS, 2, 10),
          NO_ATTK, NO_ATTK),
        SIZ(1300, 600, MS_BURBLE, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_FLY | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_NASTY | M2_COLLECT, M3_INFRAVISIBLE,
        CLR_ORANGE),
#if 0 /* DEFERRED */
    MON("vorpal jabberwock", "{{vorpal jabberwock}}", S_JABBERWOCK,
        LVL(20, 12, -2, 50, 0), (G_GENO | 1),
        A(ATTK(AT_BITE, AD_PHYS, 3, 10), ATTK(AT_BITE, AD_PHYS, 3, 10),
          ATTK(AT_CLAW, AD_PHYS, 3, 10), ATTK(AT_CLAW, AD_PHYS, 3, 10),
          NO_ATTK, NO_ATTK),
        SIZ(1300, 600, MS_BURBLE, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_FLY | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_NASTY | M2_COLLECT, M3_INFRAVISIBLE,
        HI_LORD),
#endif
    /*
     * Kops
     */
    MON("Keystone Kop", "{{Keystone Kop}}", S_KOP, LVL(1, 6, 10, 10, 9),
        (G_GENO | G_LGROUP | G_NOGEN),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 200, MS_ARREST, MZ_HUMAN), 0, 0, M1_HUMANOID,
        M2_HUMAN | M2_WANDER | M2_HOSTILE | M2_MALE | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_BLUE),
    MON("Kop Sergeant", "{{Kop Sergeant}}", S_KOP, LVL(2, 8, 10, 10, 10),
        (G_GENO | G_SGROUP | G_NOGEN),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 200, MS_ARREST, MZ_HUMAN), 0, 0, M1_HUMANOID,
        M2_HUMAN | M2_WANDER | M2_HOSTILE | M2_STRONG | M2_MALE | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_BLUE),
    MON("Kop Lieutenant", "{{Kop Lieutenant}}", S_KOP, LVL(3, 10, 10, 20, 11), (G_GENO | G_NOGEN),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 200, MS_ARREST, MZ_HUMAN), 0, 0, M1_HUMANOID,
        M2_HUMAN | M2_WANDER | M2_HOSTILE | M2_STRONG | M2_MALE | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_CYAN),
    MON("Kop Kaptain", "{{Kop Kaptain}}", S_KOP, LVL(4, 12, 10, 20, 12), (G_GENO | G_NOGEN),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 200, MS_ARREST, MZ_HUMAN), 0, 0, M1_HUMANOID,
        M2_HUMAN | M2_WANDER | M2_HOSTILE | M2_STRONG | M2_MALE | M2_COLLECT,
        M3_INFRAVISIBLE, HI_LORD),
    /*
     * Liches
     */
    MON("lich", "{{lich}}", S_LICH, LVL(11, 6, 0, 30, -9), (G_GENO | G_NOCORPSE | 1),
        A(ATTK(AT_TUCH, AD_COLD, 1, 10), ATTK(AT_MAGC, AD_SPEL, 0, 0),
          NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1200, 100, MS_MUMBLE, MZ_HUMAN), MR_COLD | MR_SLEEP | MR_POISON,
        MR_COLD, M1_BREATHLESS | M1_HUMANOID | M1_POIS | M1_REGEN,
        M2_UNDEAD | M2_HOSTILE | M2_MAGIC, M3_INFRAVISION, CLR_BROWN),
    MON("demilich", "{{demilich}}", S_LICH, LVL(14, 9, -2, 60, -12),
        (G_GENO | G_NOCORPSE | 1),
        A(ATTK(AT_TUCH, AD_COLD, 3, 4), ATTK(AT_MAGC, AD_SPEL, 0, 0), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1200, 100, MS_MUMBLE, MZ_HUMAN), MR_COLD | MR_SLEEP | MR_POISON,
        MR_COLD, M1_BREATHLESS | M1_HUMANOID | M1_POIS | M1_REGEN,
        M2_UNDEAD | M2_HOSTILE | M2_MAGIC, M3_INFRAVISION, CLR_RED),
    MON("master lich", "{{master lich}}", S_LICH, LVL(17, 9, -4, 90, -15),
        (G_HELL | G_GENO | G_NOCORPSE | 1),
        A(ATTK(AT_TUCH, AD_COLD, 3, 6), ATTK(AT_MAGC, AD_SPEL, 0, 0), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1200, 100, MS_MUMBLE, MZ_HUMAN),
        MR_FIRE | MR_COLD | MR_SLEEP | MR_POISON, MR_FIRE | MR_COLD,
        M1_BREATHLESS | M1_HUMANOID | M1_POIS | M1_REGEN,
        M2_UNDEAD | M2_HOSTILE | M2_MAGIC, M3_WANTSBOOK | M3_INFRAVISION,
        HI_LORD),
    MON("arch-lich", "{{arch-lich}}", S_LICH, LVL(25, 9, -6, 90, -15),
        (G_HELL | G_GENO | G_NOCORPSE | 1),
        A(ATTK(AT_TUCH, AD_COLD, 5, 6), ATTK(AT_MAGC, AD_SPEL, 0, 0), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1200, 100, MS_MUMBLE, MZ_HUMAN),
        MR_FIRE | MR_COLD | MR_SLEEP | MR_ELEC | MR_POISON, MR_FIRE | MR_COLD,
        M1_BREATHLESS | M1_HUMANOID | M1_POIS | M1_REGEN,
        M2_UNDEAD | M2_HOSTILE | M2_MAGIC, M3_WANTSBOOK | M3_INFRAVISION,
        HI_LORD),
    /*
     * Mummies
     */
    MON("kobold mummy", "{{kobold mummy}}", S_MUMMY, LVL(3, 8, 6, 20, -2),
        (G_GENO | G_NOCORPSE | 1), A(ATTK(AT_CLAW, AD_PHYS, 1, 4), NO_ATTK,
                                     NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(400, 50, MS_SILENT, MZ_SMALL), MR_COLD | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_POIS,
        M2_UNDEAD | M2_HOSTILE, M3_INFRAVISION, CLR_BROWN),
    MON("gnome mummy", "{{gnome mummy}}", S_MUMMY, LVL(4, 10, 6, 20, -3),
        (G_GENO | G_NOCORPSE | 1), A(ATTK(AT_CLAW, AD_PHYS, 1, 6), NO_ATTK,
                                     NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(650, 50, MS_SILENT, MZ_SMALL), MR_COLD | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_POIS,
        M2_UNDEAD | M2_HOSTILE | M2_GNOME, M3_INFRAVISION, CLR_RED),
    MON("orc mummy", "{{orc mummy}}", S_MUMMY, LVL(5, 10, 5, 20, -4),
        (G_GENO | G_NOCORPSE | 1), A(ATTK(AT_CLAW, AD_PHYS, 1, 6), NO_ATTK,
                                     NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(850, 75, MS_SILENT, MZ_HUMAN), MR_COLD | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_POIS,
        M2_UNDEAD | M2_HOSTILE | M2_ORC | M2_GREEDY | M2_JEWELS,
        M3_INFRAVISION, CLR_GRAY),
    MON("dwarf mummy", "{{dwarf mummy}}", S_MUMMY, LVL(5, 10, 5, 20, -4),
        (G_GENO | G_NOCORPSE | 1), A(ATTK(AT_CLAW, AD_PHYS, 1, 6), NO_ATTK,
                                     NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(900, 150, MS_SILENT, MZ_HUMAN), MR_COLD | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_POIS,
        M2_UNDEAD | M2_HOSTILE | M2_DWARF | M2_GREEDY | M2_JEWELS,
        M3_INFRAVISION, CLR_RED),
    MON("elf mummy", "{{elf mummy}}", S_MUMMY, LVL(6, 12, 4, 30, -5),
        (G_GENO | G_NOCORPSE | 1), A(ATTK(AT_CLAW, AD_PHYS, 2, 4), NO_ATTK,
                                     NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_ELF, 175, MS_SILENT, MZ_HUMAN), MR_COLD | MR_SLEEP | MR_POISON,
        0, M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_POIS,
        M2_UNDEAD | M2_HOSTILE | M2_ELF, M3_INFRAVISION, CLR_GREEN),
    MON("human mummy", "{{human mummy}}", S_MUMMY, LVL(6, 12, 4, 30, -5),
        (G_GENO | G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 2, 4), ATTK(AT_CLAW, AD_PHYS, 2, 4), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 200, MS_SILENT, MZ_HUMAN),
        MR_COLD | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_POIS,
        M2_UNDEAD | M2_HOSTILE, M3_INFRAVISION, CLR_GRAY),
    MON("ettin mummy", "{{ettin mummy}}", S_MUMMY, LVL(7, 12, 4, 30, -6),
        (G_GENO | G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 2, 6), ATTK(AT_CLAW, AD_PHYS, 2, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1700, 250, MS_SILENT, MZ_HUGE), MR_COLD | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_POIS,
        M2_UNDEAD | M2_HOSTILE | M2_STRONG, M3_INFRAVISION, CLR_BLUE),
    MON("giant mummy", "{{giant mummy}}", S_MUMMY, LVL(8, 14, 3, 30, -7),
        (G_GENO | G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 3, 4), ATTK(AT_CLAW, AD_PHYS, 3, 4), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(2050, 375, MS_SILENT, MZ_HUGE), MR_COLD | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_POIS,
        M2_UNDEAD | M2_HOSTILE | M2_GIANT | M2_STRONG | M2_JEWELS,
        M3_INFRAVISION, CLR_CYAN),
    /*
     * Nagas
     */
    MON("red naga hatchling", "{{red naga hatchling}}", S_NAGA, LVL(3, 10, 6, 0, 0), G_GENO,
        A(ATTK(AT_BITE, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(500, 100, MS_MUMBLE, MZ_LARGE), MR_FIRE | MR_POISON,
        MR_FIRE | MR_POISON,
        M1_NOLIMBS | M1_SLITHY | M1_THICK_HIDE | M1_NOTAKE | M1_OMNIVORE,
        M2_STRONG, M3_INFRAVISIBLE, CLR_RED),
    MON("black naga hatchling", "{{black naga hatchling}}", S_NAGA, LVL(3, 10, 6, 0, 0), G_GENO,
        A(ATTK(AT_BITE, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(500, 100, MS_MUMBLE, MZ_LARGE), MR_POISON | MR_ACID | MR_STONE,
        MR_POISON | MR_STONE, M1_NOLIMBS | M1_SLITHY | M1_THICK_HIDE | M1_ACID
                                  | M1_NOTAKE | M1_CARNIVORE,
        M2_STRONG, 0, CLR_BLACK),
    MON("golden naga hatchling", "{{golden naga hatchling}}", S_NAGA, LVL(3, 10, 6, 0, 0), G_GENO,
        A(ATTK(AT_BITE, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(500, 100, MS_MUMBLE, MZ_LARGE), MR_POISON, MR_POISON,
        M1_NOLIMBS | M1_SLITHY | M1_THICK_HIDE | M1_NOTAKE | M1_OMNIVORE,
        M2_STRONG, 0, HI_GOLD),
    MON("guardian naga hatchling", "{{guardian naga hatchling}}", S_NAGA, LVL(3, 10, 6, 0, 0), G_GENO,
        A(ATTK(AT_BITE, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(500, 100, MS_MUMBLE, MZ_LARGE), MR_POISON, MR_POISON,
        M1_NOLIMBS | M1_SLITHY | M1_THICK_HIDE | M1_NOTAKE | M1_OMNIVORE,
        M2_STRONG, 0, CLR_GREEN),
    MON("red naga", "{{red naga}}", S_NAGA, LVL(6, 12, 4, 0, -4), (G_GENO | 1),
        A(ATTK(AT_BITE, AD_PHYS, 2, 4), ATTK(AT_BREA, AD_FIRE, 2, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(2600, 400, MS_MUMBLE, MZ_HUGE), MR_FIRE | MR_POISON,
        MR_FIRE | MR_POISON, M1_NOLIMBS | M1_SLITHY | M1_THICK_HIDE
                                 | M1_OVIPAROUS | M1_NOTAKE | M1_OMNIVORE,
        M2_STRONG, M3_INFRAVISIBLE, CLR_RED),
    MON("black naga", "{{black naga}}", S_NAGA, LVL(8, 14, 2, 10, 4), (G_GENO | 1),
        A(ATTK(AT_BITE, AD_PHYS, 2, 6), ATTK(AT_SPIT, AD_ACID, 0, 0), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(2600, 400, MS_MUMBLE, MZ_HUGE), MR_POISON | MR_ACID | MR_STONE,
        MR_POISON | MR_STONE,
        M1_NOLIMBS | M1_SLITHY | M1_THICK_HIDE | M1_OVIPAROUS | M1_ACID
            | M1_NOTAKE | M1_CARNIVORE,
        M2_STRONG, 0, CLR_BLACK),
    MON("golden naga", "{{golden naga}}", S_NAGA, LVL(10, 14, 2, 70, 5), (G_GENO | 1),
        A(ATTK(AT_BITE, AD_PHYS, 2, 6), ATTK(AT_MAGC, AD_SPEL, 4, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(2600, 400, MS_MUMBLE, MZ_HUGE), MR_POISON, MR_POISON,
        M1_NOLIMBS | M1_SLITHY | M1_THICK_HIDE | M1_OVIPAROUS | M1_NOTAKE
            | M1_OMNIVORE,
        M2_STRONG, 0, HI_GOLD),
    MON("guardian naga", "{{guardian naga}}", S_NAGA, LVL(12, 16, 0, 50, 7), (G_GENO | 1),
        A(ATTK(AT_BITE, AD_PLYS, 1, 6), ATTK(AT_SPIT, AD_DRST, 1, 6),
          ATTK(AT_HUGS, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(2600, 400, MS_MUMBLE, MZ_HUGE), MR_POISON, MR_POISON,
        M1_NOLIMBS | M1_SLITHY | M1_THICK_HIDE | M1_OVIPAROUS | M1_POIS
            | M1_NOTAKE | M1_OMNIVORE,
        M2_STRONG, 0, CLR_GREEN),
    /*
     * Ogres
     */
    MON("ogre", "{{ogre}}", S_OGRE, LVL(5, 10, 5, 0, -3), (G_SGROUP | G_GENO | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 5), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1600, 500, MS_GRUNT, MZ_LARGE), 0, 0, M1_HUMANOID | M1_CARNIVORE,
        M2_STRONG | M2_GREEDY | M2_JEWELS | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BROWN),
    MON("ogre lord", "{{ogre lord}}", S_OGRE, LVL(7, 12, 3, 30, -5), (G_GENO | 2),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1700, 700, MS_GRUNT, MZ_LARGE), 0, 0, M1_HUMANOID | M1_CARNIVORE,
        M2_STRONG | M2_LORD | M2_MALE | M2_GREEDY | M2_JEWELS | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED),
    MON("ogre king", "{{ogre king}}", S_OGRE, LVL(9, 14, 4, 60, -7), (G_GENO | 2),
        A(ATTK(AT_WEAP, AD_PHYS, 3, 5), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1700, 750, MS_GRUNT, MZ_LARGE), 0, 0, M1_HUMANOID | M1_CARNIVORE,
        M2_STRONG | M2_PRINCE | M2_MALE | M2_GREEDY | M2_JEWELS | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, HI_LORD),
    /*
     * Puddings
     *
     * must be in the same order as the pudding globs in objects.c
     */
    MON("gray ooze", "{{gray ooze}}", S_PUDDING, LVL(3, 1, 8, 0, 0), (G_GENO | G_NOCORPSE | 2),
        A(ATTK(AT_BITE, AD_RUST, 2, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(500, 250, MS_SILENT, MZ_MEDIUM),
        MR_FIRE | MR_COLD | MR_POISON | MR_ACID | MR_STONE,
        MR_FIRE | MR_COLD | MR_POISON,
        M1_BREATHLESS | M1_AMORPHOUS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD
            | M1_MINDLESS | M1_OMNIVORE | M1_ACID,
        M2_HOSTILE | M2_NEUTER, 0, CLR_GRAY),
    MON("brown pudding", "{{brown pudding}}", S_PUDDING, LVL(5, 3, 8, 0, 0),
        (G_GENO | G_NOCORPSE | 1), A(ATTK(AT_BITE, AD_DCAY, 0, 0), NO_ATTK,
                                     NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(500, 250, MS_SILENT, MZ_MEDIUM),
        MR_COLD | MR_ELEC | MR_POISON | MR_ACID | MR_STONE,
        MR_COLD | MR_ELEC | MR_POISON,
        M1_BREATHLESS | M1_AMORPHOUS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD
            | M1_MINDLESS | M1_OMNIVORE | M1_ACID,
        M2_HOSTILE | M2_NEUTER, 0, CLR_BROWN),
    MON("green slime", "{{green slime}}", S_PUDDING, LVL(6, 6, 6, 0, 0),
        (G_HELL | G_GENO | G_NOCORPSE | 1),
        A(ATTK(AT_TUCH, AD_SLIM, 1, 4), ATTK(AT_NONE, AD_SLIM, 0, 0), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(400, 150, MS_SILENT, MZ_LARGE),
        MR_COLD | MR_ELEC | MR_POISON | MR_ACID | MR_STONE, 0,
        M1_BREATHLESS | M1_AMORPHOUS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD
            | M1_MINDLESS | M1_OMNIVORE | M1_ACID | M1_POIS,
        M2_HOSTILE | M2_NEUTER, 0, CLR_GREEN),
    MON("black pudding", "{{black pudding}}", S_PUDDING, LVL(10, 6, 6, 0, 0),
        (G_GENO | G_NOCORPSE | 1),
        A(ATTK(AT_BITE, AD_CORR, 3, 8), ATTK(AT_NONE, AD_CORR, 0, 0), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(900, 250, MS_SILENT, MZ_LARGE),
        MR_COLD | MR_ELEC | MR_POISON | MR_ACID | MR_STONE,
        MR_COLD | MR_ELEC | MR_POISON,
        M1_BREATHLESS | M1_AMORPHOUS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD
            | M1_MINDLESS | M1_OMNIVORE | M1_ACID,
        M2_HOSTILE | M2_NEUTER, 0, CLR_BLACK),
    /*
     * Quantum mechanics
     */
    MON("quantum mechanic", "{{quantum mechanic}}", S_QUANTMECH, LVL(7, 12, 3, 10, 0), (G_GENO | 3),
        A(ATTK(AT_CLAW, AD_TLPT, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 20, MS_HUMANOID, MZ_HUMAN), MR_POISON, 0,
        M1_HUMANOID | M1_OMNIVORE | M1_POIS | M1_TPORT, M2_HOSTILE,
        M3_INFRAVISIBLE, CLR_CYAN),
    /*
     * Rust monster or disenchanter
     */
    MON("rust monster", "{{rust monster}}", S_RUSTMONST, LVL(5, 18, 2, 0, 0), (G_GENO | 2),
        A(ATTK(AT_TUCH, AD_RUST, 0, 0), ATTK(AT_TUCH, AD_RUST, 0, 0),
          ATTK(AT_NONE, AD_RUST, 0, 0), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1000, 250, MS_SILENT, MZ_MEDIUM), 0, 0,
        M1_SWIM | M1_ANIMAL | M1_NOHANDS | M1_METALLIVORE, M2_HOSTILE,
        M3_INFRAVISIBLE, CLR_BROWN),
    MON("disenchanter", "{{disenchanter}}", S_RUSTMONST, LVL(12, 12, -10, 0, -3),
        (G_HELL | G_GENO | 2),
        A(ATTK(AT_CLAW, AD_ENCH, 4, 4), ATTK(AT_NONE, AD_ENCH, 0, 0), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(750, 200, MS_GROWL, MZ_LARGE), 0, 0, M1_ANIMAL | M1_CARNIVORE,
        M2_HOSTILE, M3_INFRAVISIBLE, CLR_BLUE),
    /*
     * Snakes
     */
    MON("garter snake", "{{garter snake}}", S_SNAKE, LVL(1, 8, 8, 0, 0), (G_LGROUP | G_GENO | 1),
        A(ATTK(AT_BITE, AD_PHYS, 1, 2), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(50, 60, MS_HISS, MZ_TINY), 0, 0,
        M1_SWIM | M1_CONCEAL | M1_NOLIMBS | M1_ANIMAL | M1_SLITHY
            | M1_OVIPAROUS | M1_CARNIVORE | M1_NOTAKE,
        0, 0, CLR_GREEN),
    MON("snake", "{{snake}}", S_SNAKE, LVL(4, 15, 3, 0, 0), (G_GENO | 2),
        A(ATTK(AT_BITE, AD_DRST, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(100, 80, MS_HISS, MZ_SMALL), MR_POISON, MR_POISON,
        M1_SWIM | M1_CONCEAL | M1_NOLIMBS | M1_ANIMAL | M1_SLITHY | M1_POIS
            | M1_OVIPAROUS | M1_CARNIVORE | M1_NOTAKE,
        M2_HOSTILE, 0, CLR_BROWN),
    MON("water moccasin", "{{water moccasin}}", S_SNAKE, LVL(4, 15, 3, 0, 0),
        (G_GENO | G_NOGEN | G_LGROUP),
        A(ATTK(AT_BITE, AD_DRST, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(150, 80, MS_HISS, MZ_SMALL), MR_POISON, MR_POISON,
        M1_SWIM | M1_CONCEAL | M1_NOLIMBS | M1_ANIMAL | M1_SLITHY | M1_POIS
            | M1_CARNIVORE | M1_OVIPAROUS | M1_NOTAKE,
        M2_HOSTILE, 0, CLR_RED),
    MON("python", "{{python}}", S_SNAKE, LVL(6, 3, 5, 0, 0), (G_GENO | 1),
        A(ATTK(AT_BITE, AD_PHYS, 1, 4), ATTK(AT_TUCH, AD_PHYS, 0, 0),
          ATTK(AT_HUGS, AD_WRAP, 1, 4), ATTK(AT_HUGS, AD_PHYS, 2, 4), NO_ATTK,
          NO_ATTK),
        SIZ(250, 100, MS_HISS, MZ_LARGE), 0, 0,
        M1_SWIM | M1_NOLIMBS | M1_ANIMAL | M1_SLITHY | M1_CARNIVORE
            | M1_OVIPAROUS | M1_NOTAKE,
        M2_HOSTILE | M2_STRONG, M3_INFRAVISION, CLR_MAGENTA),
    MON("pit viper", "{{pit viper}}", S_SNAKE, LVL(6, 15, 2, 0, 0), (G_GENO | 1),
        A(ATTK(AT_BITE, AD_DRST, 1, 4), ATTK(AT_BITE, AD_DRST, 1, 4), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(100, 60, MS_HISS, MZ_MEDIUM), MR_POISON, MR_POISON,
        M1_SWIM | M1_CONCEAL | M1_NOLIMBS | M1_ANIMAL | M1_SLITHY | M1_POIS
            | M1_CARNIVORE | M1_OVIPAROUS | M1_NOTAKE,
        M2_HOSTILE, M3_INFRAVISION, CLR_BLUE),
    MON("cobra", "{{cobra}}", S_SNAKE, LVL(6, 18, 2, 0, 0), (G_GENO | 1),
        A(ATTK(AT_BITE, AD_DRST, 2, 4), ATTK(AT_SPIT, AD_BLND, 0, 0), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(250, 100, MS_HISS, MZ_MEDIUM), MR_POISON, MR_POISON,
        M1_SWIM | M1_CONCEAL | M1_NOLIMBS | M1_ANIMAL | M1_SLITHY | M1_POIS
            | M1_CARNIVORE | M1_OVIPAROUS | M1_NOTAKE,
        M2_HOSTILE, 0, CLR_BLUE),
    /*
     * Trolls
     */
    MON("troll", "{{troll}}", S_TROLL, LVL(7, 12, 4, 0, -3), (G_GENO | 2),
        A(ATTK(AT_WEAP, AD_PHYS, 4, 2), ATTK(AT_CLAW, AD_PHYS, 4, 2),
          ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(800, 350, MS_GRUNT, MZ_LARGE), 0, 0,
        M1_HUMANOID | M1_REGEN | M1_CARNIVORE,
        M2_STRONG | M2_STALK | M2_HOSTILE, M3_INFRAVISIBLE | M3_INFRAVISION,
        CLR_BROWN),
    MON("ice troll", "{{ice troll}}", S_TROLL, LVL(9, 10, 2, 20, -3), (G_NOHELL | G_GENO | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 6), ATTK(AT_CLAW, AD_COLD, 2, 6),
          ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1000, 300, MS_GRUNT, MZ_LARGE), MR_COLD, MR_COLD,
        M1_HUMANOID | M1_REGEN | M1_CARNIVORE,
        M2_STRONG | M2_STALK | M2_HOSTILE, M3_INFRAVISIBLE | M3_INFRAVISION,
        CLR_WHITE),
    MON("rock troll", "{{rock troll}}", S_TROLL, LVL(9, 12, 0, 0, -3), (G_GENO | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 3, 6), ATTK(AT_CLAW, AD_PHYS, 2, 8),
          ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1200, 300, MS_GRUNT, MZ_LARGE), 0, 0,
        M1_HUMANOID | M1_REGEN | M1_CARNIVORE,
        M2_STRONG | M2_STALK | M2_HOSTILE | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_CYAN),
    MON("water troll", "{{water troll}}", S_TROLL, LVL(11, 14, 4, 40, -3), (G_NOGEN | G_GENO),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 8), ATTK(AT_CLAW, AD_PHYS, 2, 8),
          ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1200, 350, MS_GRUNT, MZ_LARGE), 0, 0,
        M1_HUMANOID | M1_REGEN | M1_CARNIVORE | M1_SWIM,
        M2_STRONG | M2_STALK | M2_HOSTILE, M3_INFRAVISIBLE | M3_INFRAVISION,
        CLR_BLUE),
    MON("Olog-hai", "{{Olog-hai}}", S_TROLL, LVL(13, 12, -4, 0, -7), (G_GENO | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 3, 6), ATTK(AT_CLAW, AD_PHYS, 2, 8),
          ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1500, 400, MS_GRUNT, MZ_LARGE), 0, 0,
        M1_HUMANOID | M1_REGEN | M1_CARNIVORE,
        M2_STRONG | M2_STALK | M2_HOSTILE | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, HI_LORD),
    /*
     * Umber hulk
     */
    MON("umber hulk", "{{umber hulk}}", S_UMBER, LVL(9, 6, 2, 25, 0), (G_GENO | 2),
        A(ATTK(AT_CLAW, AD_PHYS, 3, 4), ATTK(AT_CLAW, AD_PHYS, 3, 4),
          ATTK(AT_BITE, AD_PHYS, 2, 5), ATTK(AT_GAZE, AD_CONF, 0, 0), NO_ATTK,
          NO_ATTK),
        SIZ(1200, 500, MS_SILENT, MZ_LARGE), 0, 0, M1_TUNNEL | M1_CARNIVORE,
        M2_STRONG, M3_INFRAVISIBLE, CLR_BROWN),
    /*
     * Vampires
     */
    MON("vampire", "{{vampire}}", S_VAMPIRE, LVL(10, 12, 1, 25, -8),
        (G_GENO | G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 6), ATTK(AT_BITE, AD_DRLI, 1, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_VAMPIRE, MZ_HUMAN), MR_SLEEP | MR_POISON, 0,
        M1_FLY | M1_BREATHLESS | M1_HUMANOID | M1_POIS | M1_REGEN,
        M2_UNDEAD | M2_STALK | M2_HOSTILE | M2_STRONG | M2_NASTY
            | M2_SHAPESHIFTER,
        M3_INFRAVISIBLE, CLR_RED),
    MON("vampire lord", "{{vampire lord}}", S_VAMPIRE, LVL(12, 14, 0, 50, -9),
        (G_GENO | G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 8), ATTK(AT_BITE, AD_DRLI, 1, 8), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_VAMPIRE, MZ_HUMAN), MR_SLEEP | MR_POISON, 0,
        M1_FLY | M1_BREATHLESS | M1_HUMANOID | M1_POIS | M1_REGEN,
        M2_UNDEAD | M2_STALK | M2_HOSTILE | M2_STRONG | M2_NASTY | M2_LORD
            | M2_MALE | M2_SHAPESHIFTER,
        M3_INFRAVISIBLE, CLR_BLUE),
#if 0 /* DEFERRED */
    MON("vampire mage", "{{vampire mage}}", S_VAMPIRE,
        LVL(20, 14, -4, 50, -9), (G_GENO | G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_DRLI, 2, 8), ATTK(AT_BITE, AD_DRLI, 1, 8),
          ATTK(AT_MAGC, AD_SPEL, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_VAMPIRE, MZ_HUMAN), MR_SLEEP | MR_POISON, 0,
        M1_FLY | M1_BREATHLESS | M1_HUMANOID | M1_POIS | M1_REGEN,
        M2_UNDEAD | M2_STALK | M2_HOSTILE | M2_STRONG | M2_NASTY | M2_LORD
          | M2_MALE | M2_MAGIC | M2_SHAPESHIFTER,
        M3_INFRAVISIBLE, HI_ZAP),
#endif
    MON("Vlad the Impaler", "{{Vlad the Impaler}}", S_VAMPIRE, LVL(28, 26, -6, 80, -10),
        (G_NOGEN | G_NOCORPSE | G_UNIQ),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 10), ATTK(AT_BITE, AD_DRLI, 1, 12),
          NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_VAMPIRE, MZ_HUMAN), MR_SLEEP | MR_POISON, 0,
        M1_FLY | M1_BREATHLESS | M1_HUMANOID | M1_POIS | M1_REGEN,
        M2_NOPOLY | M2_UNDEAD | M2_STALK | M2_HOSTILE | M2_PNAME | M2_STRONG
            | M2_NASTY | M2_PRINCE | M2_MALE | M2_SHAPESHIFTER,
        M3_WAITFORU | M3_WANTSCAND | M3_INFRAVISIBLE, HI_LORD),
    /*
     * Wraiths
     */
    MON("barrow wight", "{{barrow wight}}", S_WRAITH, LVL(3, 12, 5, 5, -3),
        (G_GENO | G_NOCORPSE | 1),
        A(ATTK(AT_WEAP, AD_DRLI, 0, 0), ATTK(AT_MAGC, AD_SPEL, 0, 0),
          ATTK(AT_CLAW, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1200, 0, MS_SPELL, MZ_HUMAN), MR_COLD | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_HUMANOID,
        M2_UNDEAD | M2_STALK | M2_HOSTILE | M2_COLLECT, 0, CLR_GRAY),
    MON("wraith", "{{wraith}}", S_WRAITH, LVL(6, 12, 4, 15, -6), (G_GENO | 2),
        A(ATTK(AT_TUCH, AD_DRLI, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(0, 0, MS_SILENT, MZ_HUMAN),
        MR_COLD | MR_SLEEP | MR_POISON | MR_STONE, 0,
        M1_BREATHLESS | M1_FLY | M1_HUMANOID | M1_UNSOLID,
        M2_UNDEAD | M2_STALK | M2_HOSTILE, 0, CLR_BLACK),
    MON("Nazgul", "{{Nazgul}}", S_WRAITH, LVL(13, 12, 0, 25, -17),
        (G_GENO | G_NOCORPSE | 1),
        A(ATTK(AT_WEAP, AD_DRLI, 1, 4), ATTK(AT_BREA, AD_SLEE, 2, 25),
          NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 0, MS_SPELL, MZ_HUMAN), MR_COLD | MR_SLEEP | MR_POISON,
        0, M1_BREATHLESS | M1_HUMANOID,
        M2_NOPOLY | M2_UNDEAD | M2_STALK | M2_STRONG | M2_HOSTILE | M2_MALE
            | M2_COLLECT,
        0, HI_LORD),
    /*
     * Xorn
     */
    MON("xorn", "{{xorn}}", S_XORN, LVL(8, 9, -2, 20, 0), (G_GENO | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 3), ATTK(AT_CLAW, AD_PHYS, 1, 3),
          ATTK(AT_CLAW, AD_PHYS, 1, 3), ATTK(AT_BITE, AD_PHYS, 4, 6), NO_ATTK,
          NO_ATTK),
        SIZ(1200, 700, MS_ROAR, MZ_MEDIUM), MR_FIRE | MR_COLD | MR_STONE,
        MR_STONE,
        M1_BREATHLESS | M1_WALLWALK | M1_THICK_HIDE | M1_METALLIVORE,
        M2_HOSTILE | M2_STRONG, 0, CLR_BROWN),
    /*
     * Apelike beasts
     */
    /* tameable via banana; does not grow up into ape...
       not flagged as domestic, so no guilt penalty for eating non-pet one */
    MON("monkey", "{{monkey}}", S_YETI, LVL(2, 12, 6, 0, 0), (G_GENO | 1),
        A(ATTK(AT_CLAW, AD_SITM, 0, 0), ATTK(AT_BITE, AD_PHYS, 1, 3), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(100, 50, MS_GROWL, MZ_SMALL), 0, 0,
        M1_ANIMAL | M1_HUMANOID | M1_OMNIVORE, 0, M3_INFRAVISIBLE, CLR_GRAY),
    MON("ape", "{{ape}}", S_YETI, LVL(4, 12, 6, 0, 0), (G_GENO | G_SGROUP | 2),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 3), ATTK(AT_CLAW, AD_PHYS, 1, 3),
          ATTK(AT_BITE, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1100, 500, MS_GROWL, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_HUMANOID | M1_OMNIVORE, M2_STRONG, M3_INFRAVISIBLE,
        CLR_BROWN),
    MON("owlbear", "{{owlbear}}", S_YETI, LVL(5, 12, 5, 0, 0), (G_GENO | 3),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 6), ATTK(AT_CLAW, AD_PHYS, 1, 6),
          ATTK(AT_HUGS, AD_PHYS, 2, 8), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1700, 700, MS_ROAR, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_HUMANOID | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_NASTY, M3_INFRAVISIBLE, CLR_BROWN),
    MON("yeti", "{{yeti}}", S_YETI, LVL(5, 15, 6, 0, 0), (G_GENO | 2),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 6), ATTK(AT_CLAW, AD_PHYS, 1, 6),
          ATTK(AT_BITE, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1600, 700, MS_GROWL, MZ_LARGE), MR_COLD, MR_COLD,
        M1_ANIMAL | M1_HUMANOID | M1_CARNIVORE, M2_HOSTILE | M2_STRONG,
        M3_INFRAVISIBLE, CLR_WHITE),
    MON("carnivorous ape", "{{carnivorous ape}}", S_YETI, LVL(6, 12, 6, 0, 0), (G_GENO | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4),
          ATTK(AT_HUGS, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1250, 550, MS_GROWL, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_HUMANOID | M1_CARNIVORE, M2_HOSTILE | M2_STRONG,
        M3_INFRAVISIBLE, CLR_BLACK),
    MON("sasquatch", "{{sasquatch}}", S_YETI, LVL(7, 15, 6, 0, 2), (G_GENO | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 6), ATTK(AT_CLAW, AD_PHYS, 1, 6),
          ATTK(AT_KICK, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1550, 750, MS_GROWL, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_HUMANOID | M1_SEE_INVIS | M1_OMNIVORE, M2_STRONG,
        M3_INFRAVISIBLE, CLR_GRAY),
    /*
     * Zombies
     */
    MON("kobold zombie", "{{kobold zombie}}", S_ZOMBIE, LVL(0, 6, 10, 0, -2),
        (G_GENO | G_NOCORPSE | 1), A(ATTK(AT_CLAW, AD_PHYS, 1, 4), NO_ATTK,
                                     NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(400, 50, MS_SILENT, MZ_SMALL), MR_COLD | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_POIS,
        M2_UNDEAD | M2_STALK | M2_HOSTILE, M3_INFRAVISION, CLR_BROWN),
    MON("gnome zombie", "{{gnome zombie}}", S_ZOMBIE, LVL(1, 6, 10, 0, -2),
        (G_GENO | G_NOCORPSE | 1), A(ATTK(AT_CLAW, AD_PHYS, 1, 5), NO_ATTK,
                                     NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(650, 50, MS_SILENT, MZ_SMALL), MR_COLD | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_POIS,
        M2_UNDEAD | M2_STALK | M2_HOSTILE | M2_GNOME, M3_INFRAVISION,
        CLR_BROWN),
    MON("orc zombie", "{{orc zombie}}", S_ZOMBIE, LVL(2, 6, 9, 0, -3),
        (G_GENO | G_SGROUP | G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(850, 75, MS_SILENT, MZ_HUMAN), MR_COLD | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_POIS,
        M2_UNDEAD | M2_STALK | M2_HOSTILE | M2_ORC, M3_INFRAVISION, CLR_GRAY),
    MON("dwarf zombie", "{{dwarf zombie}}", S_ZOMBIE, LVL(2, 6, 9, 0, -3),
        (G_GENO | G_SGROUP | G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(900, 150, MS_SILENT, MZ_HUMAN), MR_COLD | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_POIS,
        M2_UNDEAD | M2_STALK | M2_HOSTILE | M2_DWARF, M3_INFRAVISION,
        CLR_RED),
    MON("elf zombie", "{{elf zombie}}", S_ZOMBIE, LVL(3, 6, 9, 0, -3),
        (G_GENO | G_SGROUP | G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 7), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_ELF, 175, MS_SILENT, MZ_HUMAN), MR_COLD | MR_SLEEP | MR_POISON,
        0, M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID,
        M2_UNDEAD | M2_STALK | M2_HOSTILE | M2_ELF, M3_INFRAVISION,
        CLR_GREEN),
    MON("human zombie", "{{human zombie}}", S_ZOMBIE, LVL(4, 6, 8, 0, -3),
        (G_GENO | G_SGROUP | G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 200, MS_SILENT, MZ_HUMAN),
        MR_COLD | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID,
        M2_UNDEAD | M2_STALK | M2_HOSTILE, M3_INFRAVISION, HI_DOMESTIC),
    MON("ettin zombie", "{{ettin zombie}}", S_ZOMBIE, LVL(6, 8, 6, 0, -4),
        (G_GENO | G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 10), ATTK(AT_CLAW, AD_PHYS, 1, 10),
          NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1700, 250, MS_SILENT, MZ_HUGE), MR_COLD | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID,
        M2_UNDEAD | M2_STALK | M2_HOSTILE | M2_STRONG, M3_INFRAVISION,
        CLR_BLUE),
    MON("ghoul", "{{ghoul}}", S_ZOMBIE, LVL(3, 6, 10, 0, -2), (G_GENO | G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PLYS, 1, 2), ATTK(AT_CLAW, AD_PHYS, 1, 3), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(400, 50, MS_SILENT, MZ_SMALL), MR_COLD | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_POIS | M1_OMNIVORE,
        M2_UNDEAD | M2_WANDER | M2_HOSTILE, M3_INFRAVISION, CLR_BLACK),
    MON("giant zombie", "{{giant zombie}}", S_ZOMBIE, LVL(8, 8, 6, 0, -4),
        (G_GENO | G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 2, 8), ATTK(AT_CLAW, AD_PHYS, 2, 8), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(2050, 375, MS_SILENT, MZ_HUGE), MR_COLD | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID,
        M2_UNDEAD | M2_STALK | M2_HOSTILE | M2_GIANT | M2_STRONG,
        M3_INFRAVISION, CLR_CYAN),
    MON("skeleton", "{{skeleton}}", S_ZOMBIE, LVL(12, 8, 4, 0, 0), (G_NOCORPSE | G_NOGEN),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 6), ATTK(AT_TUCH, AD_SLOW, 1, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(300, 5, MS_BONES, MZ_HUMAN),
        MR_COLD | MR_SLEEP | MR_POISON | MR_STONE, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_THICK_HIDE,
        M2_UNDEAD | M2_WANDER | M2_HOSTILE | M2_STRONG | M2_COLLECT
            | M2_NASTY,
        M3_INFRAVISION, CLR_WHITE),
    /*
     * golems
     */
    MON("straw golem", "{{straw golem}}", S_GOLEM, LVL(3, 12, 10, 0, 0), (G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 2), ATTK(AT_CLAW, AD_PHYS, 1, 2), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(400, 0, MS_SILENT, MZ_LARGE), MR_COLD | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID, M2_HOSTILE | M2_NEUTER, 0,
        CLR_YELLOW),
    MON("paper golem", "{{paper golem}}", S_GOLEM, LVL(3, 12, 10, 0, 0), (G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 3), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(400, 0, MS_SILENT, MZ_LARGE), MR_COLD | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID, M2_HOSTILE | M2_NEUTER, 0,
        HI_PAPER),
    MON("rope golem", "{{rope golem}}", S_GOLEM, LVL(4, 9, 8, 0, 0), (G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4),
          ATTK(AT_HUGS, AD_PHYS, 6, 1), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(450, 0, MS_SILENT, MZ_LARGE), MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID, M2_HOSTILE | M2_NEUTER, 0,
        CLR_BROWN),
    MON("gold golem", "{{gold golem}}", S_GOLEM, LVL(5, 9, 6, 0, 0), (G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 2, 3), ATTK(AT_CLAW, AD_PHYS, 2, 3), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(450, 0, MS_SILENT, MZ_LARGE), MR_SLEEP | MR_POISON | MR_ACID, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_THICK_HIDE,
        M2_HOSTILE | M2_NEUTER, 0, HI_GOLD),
    MON("leather golem", "{{leather golem}}", S_GOLEM, LVL(6, 6, 6, 0, 0), (G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 6), ATTK(AT_CLAW, AD_PHYS, 1, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(800, 0, MS_SILENT, MZ_LARGE), MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID, M2_HOSTILE | M2_NEUTER, 0,
        HI_LEATHER),
    MON("wood golem", "{{wood golem}}", S_GOLEM, LVL(7, 3, 4, 0, 0), (G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 3, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(900, 0, MS_SILENT, MZ_LARGE), MR_COLD | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_THICK_HIDE,
        M2_HOSTILE | M2_NEUTER, 0, HI_WOOD),
    MON("flesh golem", "{{flesh golem}}", S_GOLEM, LVL(9, 8, 9, 30, 0), (1),
        A(ATTK(AT_CLAW, AD_PHYS, 2, 8), ATTK(AT_CLAW, AD_PHYS, 2, 8), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1400, 600, MS_SILENT, MZ_LARGE),
        MR_FIRE | MR_COLD | MR_ELEC | MR_SLEEP | MR_POISON,
        MR_FIRE | MR_COLD | MR_ELEC | MR_SLEEP | MR_POISON,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID, M2_HOSTILE | M2_STRONG, 0,
        CLR_RED),
    MON("clay golem", "{{clay golem}}", S_GOLEM, LVL(11, 7, 7, 40, 0), (G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 3, 10), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1550, 0, MS_SILENT, MZ_LARGE), MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_THICK_HIDE,
        M2_HOSTILE | M2_STRONG, 0, CLR_BROWN),
    MON("stone golem", "{{stone golem}}", S_GOLEM, LVL(14, 6, 5, 50, 0), (G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 3, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1900, 0, MS_SILENT, MZ_LARGE), MR_SLEEP | MR_POISON | MR_STONE, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_THICK_HIDE,
        M2_HOSTILE | M2_STRONG, 0, CLR_GRAY),
    MON("glass golem", "{{glass golem}}", S_GOLEM, LVL(16, 6, 1, 50, 0), (G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 2, 8), ATTK(AT_CLAW, AD_PHYS, 2, 8), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1800, 0, MS_SILENT, MZ_LARGE), MR_SLEEP | MR_POISON | MR_ACID, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_THICK_HIDE,
        M2_HOSTILE | M2_STRONG, 0, CLR_CYAN),
    MON("iron golem", "{{iron golem}}", S_GOLEM, LVL(18, 6, 3, 60, 0), (G_NOCORPSE | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 4, 10), ATTK(AT_BREA, AD_DRST, 4, 6),
          NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(2000, 0, MS_SILENT, MZ_LARGE),
        MR_FIRE | MR_COLD | MR_ELEC | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_THICK_HIDE | M1_POIS,
        M2_HOSTILE | M2_STRONG | M2_COLLECT, 0, HI_METAL),
    /*
     * humans, including elves and were-critters
     */
    MON("human", "{{human}}", S_HUMAN, LVL(0, 12, 10, 0, 0), G_NOGEN, /* for corpses */
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_COLLECT, M3_INFRAVISIBLE,
        HI_DOMESTIC),
    MON("wererat", "{{wererat}}", S_HUMAN, LVL(2, 12, 10, 10, -7), (1),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_WERE, MZ_HUMAN), MR_POISON, 0,
        M1_HUMANOID | M1_POIS | M1_REGEN | M1_OMNIVORE,
        M2_NOPOLY | M2_WERE | M2_HOSTILE | M2_HUMAN | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_BROWN),
    MON("werejackal", "{{werejackal}}", S_HUMAN, LVL(2, 12, 10, 10, -7), (1),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_WERE, MZ_HUMAN), MR_POISON, 0,
        M1_HUMANOID | M1_POIS | M1_REGEN | M1_OMNIVORE,
        M2_NOPOLY | M2_WERE | M2_HOSTILE | M2_HUMAN | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_RED),
    MON("werewolf", "{{werewolf}}", S_HUMAN, LVL(5, 12, 10, 20, -7), (1),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_WERE, MZ_HUMAN), MR_POISON, 0,
        M1_HUMANOID | M1_POIS | M1_REGEN | M1_OMNIVORE,
        M2_NOPOLY | M2_WERE | M2_HOSTILE | M2_HUMAN | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_ORANGE),
    MON("elf", "{{elf}}", S_HUMAN, LVL(10, 12, 10, 2, -3), G_NOGEN, /* for corpses */
        A(ATTK(AT_WEAP, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_ELF, 350, MS_HUMANOID, MZ_HUMAN), MR_SLEEP, MR_SLEEP,
        M1_HUMANOID | M1_OMNIVORE | M1_SEE_INVIS,
        M2_NOPOLY | M2_ELF | M2_STRONG | M2_COLLECT,
        M3_INFRAVISION | M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("Woodland-elf", "{{Woodland-elf}}", S_HUMAN, LVL(4, 12, 10, 10, -5),
        (G_GENO | G_SGROUP | 2), A(ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK,
                                   NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_ELF, 350, MS_HUMANOID, MZ_HUMAN), MR_SLEEP, MR_SLEEP,
        M1_HUMANOID | M1_OMNIVORE | M1_SEE_INVIS, M2_ELF | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_GREEN),
    MON("Green-elf", "{{Green-elf}}", S_HUMAN, LVL(5, 12, 10, 10, -6), (G_GENO | G_SGROUP | 2),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_ELF, 350, MS_HUMANOID, MZ_HUMAN), MR_SLEEP, MR_SLEEP,
        M1_HUMANOID | M1_OMNIVORE | M1_SEE_INVIS, M2_ELF | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BRIGHT_GREEN),
    MON("Grey-elf", "{{Grey-elf}}", S_HUMAN, LVL(6, 12, 10, 10, -7), (G_GENO | G_SGROUP | 2),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_ELF, 350, MS_HUMANOID, MZ_HUMAN), MR_SLEEP, MR_SLEEP,
        M1_HUMANOID | M1_OMNIVORE | M1_SEE_INVIS, M2_ELF | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_GRAY),
    MON("elf-lord", "{{elf-lord}}", S_HUMAN, LVL(8, 12, 10, 20, -9), (G_GENO | G_SGROUP | 2),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 4), ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_ELF, 350, MS_HUMANOID, MZ_HUMAN), MR_SLEEP, MR_SLEEP,
        M1_HUMANOID | M1_OMNIVORE | M1_SEE_INVIS,
        M2_ELF | M2_STRONG | M2_LORD | M2_MALE | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BRIGHT_BLUE),
    MON("Elvenking", "{{Elvenking}}", S_HUMAN, LVL(9, 12, 10, 25, -10), (G_GENO | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 4), ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_ELF, 350, MS_HUMANOID, MZ_HUMAN), MR_SLEEP, MR_SLEEP,
        M1_HUMANOID | M1_OMNIVORE | M1_SEE_INVIS,
        M2_ELF | M2_STRONG | M2_PRINCE | M2_MALE | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, HI_LORD),
    MON("doppelganger", "{{doppelganger}}", S_HUMAN, LVL(9, 12, 5, 20, 0), (G_GENO | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 12), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_IMITATE, MZ_HUMAN), MR_SLEEP, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_HOSTILE | M2_STRONG | M2_COLLECT
            | M2_SHAPESHIFTER,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("shopkeeper", "{{shopkeeper}}", S_HUMAN, LVL(12, 18, 0, 50, 0), G_NOGEN,
        A(ATTK(AT_WEAP, AD_PHYS, 4, 4), ATTK(AT_WEAP, AD_PHYS, 4, 4), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_SELL, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE, M2_NOPOLY | M2_HUMAN | M2_PEACEFUL
                                       | M2_STRONG | M2_COLLECT | M2_MAGIC,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("guard", "{{guard}}", S_HUMAN, LVL(12, 12, 10, 40, 10), G_NOGEN,
        A(ATTK(AT_WEAP, AD_PHYS, 4, 10), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_GUARD, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_MERC | M2_PEACEFUL | M2_STRONG | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_BLUE),
    MON("prisoner", "{{prisoner}}", S_HUMAN, LVL(12, 12, 10, 0, 0),
        G_NOGEN, /* for special levels */
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_DJINNI, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_COLLECT,
        M3_INFRAVISIBLE | M3_CLOSE, HI_DOMESTIC),
    MON("Oracle", "{{Oracle}}", S_HUMAN, LVL(12, 0, 0, 50, 0), (G_NOGEN | G_UNIQ),
        A(ATTK(AT_NONE, AD_MAGM, 0, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_ORACLE, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_FEMALE, M3_INFRAVISIBLE,
        HI_ZAP),
    /* aligned priests always have the epri extension attached;
       individual instantiations should always have either ispriest
       or isminion set */
    MON("aligned priest", "{{aligned priest}}", S_HUMAN, LVL(12, 12, 10, 50, 0), G_NOGEN,
        A(ATTK(AT_WEAP, AD_PHYS, 4, 10), ATTK(AT_KICK, AD_PHYS, 1, 4),
          ATTK(AT_MAGC, AD_CLRC, 0, 0), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_PRIEST, MZ_HUMAN), MR_ELEC, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_LORD | M2_PEACEFUL | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_WHITE),
    /* high priests always have epri and always have ispriest set */
    MON("high priest", "{{high priest}}", S_HUMAN, LVL(25, 15, 7, 70, 0), (G_NOGEN | G_UNIQ),
        A(ATTK(AT_WEAP, AD_PHYS, 4, 10), ATTK(AT_KICK, AD_PHYS, 2, 8),
          ATTK(AT_MAGC, AD_CLRC, 2, 8), ATTK(AT_MAGC, AD_CLRC, 2, 8), NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_PRIEST, MZ_HUMAN),
        MR_FIRE | MR_ELEC | MR_SLEEP | MR_POISON, 0,
        M1_HUMANOID | M1_SEE_INVIS | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_MINION | M2_PRINCE | M2_NASTY | M2_COLLECT
            | M2_MAGIC,
        M3_INFRAVISIBLE, CLR_WHITE),
    MON("soldier", "{{soldier}}", S_HUMAN, LVL(6, 10, 10, 0, -2), (G_SGROUP | G_GENO | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_SOLDIER, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE, M2_NOPOLY | M2_HUMAN | M2_MERC | M2_STALK
                                       | M2_HOSTILE | M2_STRONG | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_GRAY),
    MON("sergeant", "{{sergeant}}", S_HUMAN, LVL(8, 10, 10, 5, -3), (G_SGROUP | G_GENO | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_SOLDIER, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE, M2_NOPOLY | M2_HUMAN | M2_MERC | M2_STALK
                                       | M2_HOSTILE | M2_STRONG | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_RED),
    MON("nurse", "{{nurse}}", S_HUMAN, LVL(11, 6, 0, 0, 0), (G_GENO | 3),
        A(ATTK(AT_CLAW, AD_HEAL, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_NURSE, MZ_HUMAN), MR_POISON, MR_POISON,
        M1_HUMANOID | M1_OMNIVORE, M2_NOPOLY | M2_HUMAN | M2_HOSTILE,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("lieutenant", "{{lieutenant}}", S_HUMAN, LVL(10, 10, 10, 15, -4), (G_GENO | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 3, 4), ATTK(AT_WEAP, AD_PHYS, 3, 4), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_SOLDIER, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE, M2_NOPOLY | M2_HUMAN | M2_MERC | M2_STALK
                                       | M2_HOSTILE | M2_STRONG | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_GREEN),
    MON("captain", "{{captain}}", S_HUMAN, LVL(12, 10, 10, 15, -5), (G_GENO | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 4, 4), ATTK(AT_WEAP, AD_PHYS, 4, 4), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_SOLDIER, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE, M2_NOPOLY | M2_HUMAN | M2_MERC | M2_STALK
                                       | M2_HOSTILE | M2_STRONG | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_BLUE),
    /* Keep these separate - some of the mkroom code assumes that
     * all the soldiers are contiguous.
     */
    MON("watchman", "{{watchman}}", S_HUMAN, LVL(6, 10, 10, 0, -2),
        (G_SGROUP | G_NOGEN | G_GENO | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_SOLDIER, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE, M2_NOPOLY | M2_HUMAN | M2_MERC | M2_STALK
                                       | M2_PEACEFUL | M2_STRONG | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_GRAY),
    MON("watch captain", "{{watch captain}}", S_HUMAN, LVL(10, 10, 10, 15, -4),
        (G_NOGEN | G_GENO | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 3, 4), ATTK(AT_WEAP, AD_PHYS, 3, 4), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_SOLDIER, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE, M2_NOPOLY | M2_HUMAN | M2_MERC | M2_STALK
                                       | M2_PEACEFUL | M2_STRONG | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_GREEN),
    /* Unique humans not tied to quests.
     */
    MON("Medusa", "{{Medusa}}", S_HUMAN, LVL(20, 12, 2, 50, -15), (G_NOGEN | G_UNIQ),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 4), ATTK(AT_CLAW, AD_PHYS, 1, 8),
          ATTK(AT_GAZE, AD_STON, 0, 0), ATTK(AT_BITE, AD_DRST, 1, 6), NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_HISS, MZ_LARGE), MR_POISON | MR_STONE,
        MR_POISON | MR_STONE, M1_FLY | M1_SWIM | M1_AMPHIBIOUS | M1_HUMANOID
                                  | M1_POIS | M1_OMNIVORE,
        M2_NOPOLY | M2_HOSTILE | M2_STRONG | M2_PNAME | M2_FEMALE,
        M3_WAITFORU | M3_INFRAVISIBLE, CLR_BRIGHT_GREEN),
    MON("Wizard of Yendor", "{{Wizard of Yendor}}", S_HUMAN, LVL(30, 12, -8, 100, A_NONE),
        (G_NOGEN | G_UNIQ),
        A(ATTK(AT_CLAW, AD_SAMU, 2, 12), ATTK(AT_MAGC, AD_SPEL, 0, 0),
          NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_CUSS, MZ_HUMAN), MR_FIRE | MR_POISON,
        MR_FIRE | MR_POISON,
        M1_FLY | M1_BREATHLESS | M1_HUMANOID | M1_REGEN | M1_SEE_INVIS
            | M1_TPORT | M1_TPORT_CNTRL | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_HOSTILE | M2_STRONG | M2_NASTY | M2_PRINCE
            | M2_MALE | M2_MAGIC,
        M3_COVETOUS | M3_WAITFORU | M3_INFRAVISIBLE, HI_LORD),
    MON("Croesus", "{{Croesus}}", S_HUMAN, LVL(20, 15, 0, 40, 15), (G_UNIQ | G_NOGEN),
        A(ATTK(AT_WEAP, AD_PHYS, 4, 10), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_GUARD, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_SEE_INVIS | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_STALK | M2_HOSTILE | M2_STRONG | M2_NASTY
            | M2_PNAME | M2_PRINCE | M2_MALE | M2_GREEDY | M2_JEWELS
            | M2_COLLECT | M2_MAGIC,
        M3_INFRAVISIBLE, HI_LORD),
#ifdef CHARON
    MON("Charon", "{{Charon}}", S_HUMAN, LVL(76, 18, -5, 120, 0),
        (G_HELL | G_NOCORPSE | G_NOGEN | G_UNIQ),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 8), ATTK(AT_TUCH, AD_PLYS, 1, 8), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_FERRY, MZ_HUMAN),
        MR_FIRE | MR_COLD | MR_POISON | MR_STONE, 0,
        M1_BREATHLESS | M1_SEE_INVIS | M1_HUMANOID,
        M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_PNAME | M2_MALE | M2_GREEDY
            | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_WHITE),
#endif
    /*
     * ghosts
     */
    MON("ghost", "{{ghost}}", S_GHOST, LVL(10, 3, -5, 50, -5), (G_NOCORPSE | G_NOGEN),
        A(ATTK(AT_TUCH, AD_PHYS, 1, 1), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 0, MS_SILENT, MZ_HUMAN),
        MR_COLD | MR_DISINT | MR_SLEEP | MR_POISON | MR_STONE, 0,
        M1_FLY | M1_BREATHLESS | M1_WALLWALK | M1_HUMANOID | M1_UNSOLID,
        M2_NOPOLY | M2_UNDEAD | M2_STALK | M2_HOSTILE, M3_INFRAVISION,
        CLR_GRAY),
    MON("shade", "{{shade}}", S_GHOST, LVL(12, 10, 10, 0, 0), (G_NOCORPSE | G_NOGEN),
        A(ATTK(AT_TUCH, AD_PLYS, 2, 6), ATTK(AT_TUCH, AD_SLOW, 1, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 0, MS_WAIL, MZ_HUMAN),
        MR_COLD | MR_DISINT | MR_SLEEP | MR_POISON | MR_STONE, 0,
        M1_FLY | M1_BREATHLESS | M1_WALLWALK | M1_HUMANOID | M1_UNSOLID
            | M1_SEE_INVIS,
        M2_NOPOLY | M2_UNDEAD | M2_WANDER | M2_STALK | M2_HOSTILE | M2_NASTY,
        M3_INFRAVISION, CLR_BLACK),
    /*
     * (major) demons
     */
    MON("water demon", "{{water demon}}", S_DEMON, LVL(8, 12, -4, 30, -7),
        (G_NOCORPSE | G_NOGEN),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 3), ATTK(AT_CLAW, AD_PHYS, 1, 3),
          ATTK(AT_BITE, AD_PHYS, 1, 3), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_DJINNI, MZ_HUMAN), MR_FIRE | MR_POISON, 0,
        M1_HUMANOID | M1_POIS | M1_SWIM,
        M2_NOPOLY | M2_DEMON | M2_STALK | M2_HOSTILE | M2_NASTY | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BLUE),
    /* standard demons & devils
     */
#define SEDUCTION_ATTACKS_YES                                     \
    A(ATTK(AT_BITE, AD_SSEX, 0, 0), ATTK(AT_CLAW, AD_PHYS, 1, 3), \
      ATTK(AT_CLAW, AD_PHYS, 1, 3), NO_ATTK, NO_ATTK, NO_ATTK)
#define SEDUCTION_ATTACKS_NO                                      \
    A(ATTK(AT_CLAW, AD_PHYS, 1, 3), ATTK(AT_CLAW, AD_PHYS, 1, 3), \
      ATTK(AT_BITE, AD_DRLI, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK)
    MON("succubus", "{{succubus}}", S_DEMON, LVL(6, 12, 0, 70, -9), (G_NOCORPSE | 1),
        SEDUCTION_ATTACKS_YES, SIZ(WT_HUMAN, 400, MS_SEDUCE, MZ_HUMAN),
        MR_FIRE | MR_POISON, 0, M1_HUMANOID | M1_FLY | M1_POIS,
        M2_DEMON | M2_STALK | M2_HOSTILE | M2_NASTY | M2_FEMALE,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_GRAY),
    MON("horned devil", "{{horned devil}}", S_DEMON, LVL(6, 9, -5, 50, 11),
        (G_HELL | G_NOCORPSE | 2),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4),
          ATTK(AT_BITE, AD_PHYS, 2, 3), ATTK(AT_STNG, AD_PHYS, 1, 3), NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_SILENT, MZ_HUMAN), MR_FIRE | MR_POISON, 0,
        M1_POIS | M1_THICK_HIDE, M2_DEMON | M2_STALK | M2_HOSTILE | M2_NASTY,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BROWN),
    MON("incubus", "{{incubus}}", S_DEMON, LVL(6, 12, 0, 70, -9), (G_NOCORPSE | 1),
        SEDUCTION_ATTACKS_YES, SIZ(WT_HUMAN, 400, MS_SEDUCE, MZ_HUMAN),
        MR_FIRE | MR_POISON, 0, M1_HUMANOID | M1_FLY | M1_POIS,
        M2_DEMON | M2_STALK | M2_HOSTILE | M2_NASTY | M2_MALE,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_GRAY),
    /* Used by AD&D for a type of demon, originally one of the Furies
       and spelled this way */
    MON("erinys", "{{erinys}}", S_DEMON, LVL(7, 12, 2, 30, 10),
        (G_HELL | G_NOCORPSE | G_SGROUP | 2),
        A(ATTK(AT_WEAP, AD_DRST, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_SILENT, MZ_HUMAN), MR_FIRE | MR_POISON, 0,
        M1_HUMANOID | M1_POIS,
        M2_NOPOLY | M2_DEMON | M2_STALK | M2_HOSTILE | M2_STRONG | M2_NASTY
            | M2_FEMALE | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED),
    MON("barbed devil", "{{barbed devil}}", S_DEMON, LVL(8, 12, 0, 35, 8),
        (G_HELL | G_NOCORPSE | G_SGROUP | 2),
        A(ATTK(AT_CLAW, AD_PHYS, 2, 4), ATTK(AT_CLAW, AD_PHYS, 2, 4),
          ATTK(AT_STNG, AD_PHYS, 3, 4), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_SILENT, MZ_HUMAN), MR_FIRE | MR_POISON, 0,
        M1_POIS | M1_THICK_HIDE, M2_DEMON | M2_STALK | M2_HOSTILE | M2_NASTY,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED),
    MON("marilith", "{{marilith}}", S_DEMON, LVL(7, 12, -6, 80, -12),
        (G_HELL | G_NOCORPSE | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 4), ATTK(AT_WEAP, AD_PHYS, 2, 4),
          ATTK(AT_CLAW, AD_PHYS, 2, 4), ATTK(AT_CLAW, AD_PHYS, 2, 4),
          ATTK(AT_CLAW, AD_PHYS, 2, 4), ATTK(AT_CLAW, AD_PHYS, 2, 4)),
        SIZ(WT_HUMAN, 400, MS_CUSS, MZ_LARGE), MR_FIRE | MR_POISON, 0,
        M1_HUMANOID | M1_SLITHY | M1_SEE_INVIS | M1_POIS,
        M2_DEMON | M2_STALK | M2_HOSTILE | M2_NASTY | M2_FEMALE | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED),
    MON("vrock", "{{vrock}}", S_DEMON, LVL(8, 12, 0, 50, -9),
        (G_HELL | G_NOCORPSE | G_SGROUP | 2),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4),
          ATTK(AT_CLAW, AD_PHYS, 1, 8), ATTK(AT_CLAW, AD_PHYS, 1, 8),
          ATTK(AT_BITE, AD_PHYS, 1, 6), NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_SILENT, MZ_LARGE), MR_FIRE | MR_POISON, 0,
        M1_POIS, M2_DEMON | M2_STALK | M2_HOSTILE | M2_NASTY,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED),
    MON("hezrou", "{{hezrou}}", S_DEMON, LVL(9, 6, -2, 55, -10),
        (G_HELL | G_NOCORPSE | G_SGROUP | 2),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 3), ATTK(AT_CLAW, AD_PHYS, 1, 3),
          ATTK(AT_BITE, AD_PHYS, 4, 4), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_SILENT, MZ_LARGE), MR_FIRE | MR_POISON, 0,
        M1_HUMANOID | M1_POIS, M2_DEMON | M2_STALK | M2_HOSTILE | M2_NASTY,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED),
    MON("bone devil", "{{bone devil}}", S_DEMON, LVL(9, 15, -1, 40, -9),
        (G_HELL | G_NOCORPSE | G_SGROUP | 2),
        A(ATTK(AT_WEAP, AD_PHYS, 3, 4), ATTK(AT_STNG, AD_DRST, 2, 4), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_SILENT, MZ_LARGE), MR_FIRE | MR_POISON, 0,
        M1_POIS, M2_DEMON | M2_STALK | M2_HOSTILE | M2_NASTY | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_GRAY),
    MON("ice devil", "{{ice devil}}", S_DEMON, LVL(11, 6, -4, 55, -12),
        (G_HELL | G_NOCORPSE | 2),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4),
          ATTK(AT_BITE, AD_PHYS, 2, 4), ATTK(AT_STNG, AD_COLD, 3, 4), NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_SILENT, MZ_LARGE),
        MR_FIRE | MR_COLD | MR_POISON, 0, M1_SEE_INVIS | M1_POIS,
        M2_DEMON | M2_STALK | M2_HOSTILE | M2_NASTY,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_WHITE),
    MON("nalfeshnee", "{{nalfeshnee}}", S_DEMON, LVL(11, 9, -1, 65, -11),
        (G_HELL | G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4),
          ATTK(AT_BITE, AD_PHYS, 2, 4), ATTK(AT_MAGC, AD_SPEL, 0, 0), NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_SPELL, MZ_LARGE), MR_FIRE | MR_POISON, 0,
        M1_HUMANOID | M1_POIS, M2_DEMON | M2_STALK | M2_HOSTILE | M2_NASTY,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED),
    MON("pit fiend", "{{pit fiend}}", S_DEMON, LVL(13, 6, -3, 65, -13),
        (G_HELL | G_NOCORPSE | 2),
        A(ATTK(AT_WEAP, AD_PHYS, 4, 2), ATTK(AT_WEAP, AD_PHYS, 4, 2),
          ATTK(AT_HUGS, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_GROWL, MZ_LARGE), MR_FIRE | MR_POISON, 0,
        M1_SEE_INVIS | M1_POIS,
        M2_DEMON | M2_STALK | M2_HOSTILE | M2_NASTY | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED),
    MON("sandestin", "{{sandestin}}", S_DEMON, LVL(13, 12, 4, 60, -5),
        (G_HELL | G_NOCORPSE | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 6), ATTK(AT_WEAP, AD_PHYS, 2, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1500, 400, MS_CUSS, MZ_HUMAN), MR_STONE, 0, M1_HUMANOID,
        M2_NOPOLY | M2_STALK | M2_STRONG | M2_COLLECT | M2_SHAPESHIFTER,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_GRAY),
    MON("balrog", "{{balrog}}", S_DEMON, LVL(16, 5, -2, 75, -14), (G_HELL | G_NOCORPSE | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 8, 4), ATTK(AT_WEAP, AD_PHYS, 4, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_SILENT, MZ_LARGE), MR_FIRE | MR_POISON, 0,
        M1_FLY | M1_SEE_INVIS | M1_POIS,
        M2_DEMON | M2_STALK | M2_HOSTILE | M2_STRONG | M2_NASTY | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED),
    /* Named demon lords & princes plus Arch-Devils.
     * (their order matters; see minion.c)
     */
    MON("Juiblex", "{{Juiblex}}", S_DEMON, LVL(50, 3, -7, 65, -15),
        (G_HELL | G_NOCORPSE | G_NOGEN | G_UNIQ),
        A(ATTK(AT_ENGL, AD_DISE, 4, 10), ATTK(AT_SPIT, AD_ACID, 3, 6),
          NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1500, 0, MS_GURGLE, MZ_LARGE),
        MR_FIRE | MR_POISON | MR_ACID | MR_STONE, 0,
        M1_AMPHIBIOUS | M1_AMORPHOUS | M1_NOHEAD | M1_FLY | M1_SEE_INVIS
            | M1_ACID | M1_POIS,
        M2_NOPOLY | M2_DEMON | M2_STALK | M2_HOSTILE | M2_PNAME | M2_NASTY
            | M2_LORD | M2_MALE,
        M3_WAITFORU | M3_WANTSAMUL | M3_INFRAVISION, CLR_BRIGHT_GREEN),
    MON("Yeenoghu", "{{Yeenoghu}}", S_DEMON, LVL(56, 18, -5, 80, -15),
        (G_HELL | G_NOCORPSE | G_NOGEN | G_UNIQ),
        A(ATTK(AT_WEAP, AD_PHYS, 3, 6), ATTK(AT_WEAP, AD_CONF, 2, 8),
          ATTK(AT_CLAW, AD_PLYS, 1, 6), ATTK(AT_MAGC, AD_MAGM, 2, 6), NO_ATTK,
          NO_ATTK),
        SIZ(900, 500, MS_ORC, MZ_LARGE), MR_FIRE | MR_POISON, 0,
        M1_FLY | M1_SEE_INVIS | M1_POIS,
        M2_NOPOLY | M2_DEMON | M2_STALK | M2_HOSTILE | M2_PNAME | M2_NASTY
            | M2_LORD | M2_MALE | M2_COLLECT,
        M3_WANTSAMUL | M3_INFRAVISIBLE | M3_INFRAVISION, HI_LORD),
    MON("Orcus", "{{Orcus}}", S_DEMON, LVL(66, 9, -6, 85, -20),
        (G_HELL | G_NOCORPSE | G_NOGEN | G_UNIQ),
        A(ATTK(AT_WEAP, AD_PHYS, 3, 6), ATTK(AT_CLAW, AD_PHYS, 3, 4),
          ATTK(AT_CLAW, AD_PHYS, 3, 4), ATTK(AT_MAGC, AD_SPEL, 8, 6),
          ATTK(AT_STNG, AD_DRST, 2, 4), NO_ATTK),
        SIZ(1500, 500, MS_ORC, MZ_HUGE), MR_FIRE | MR_POISON, 0,
        M1_FLY | M1_SEE_INVIS | M1_POIS,
        M2_NOPOLY | M2_DEMON | M2_STALK | M2_HOSTILE | M2_PNAME | M2_NASTY
            | M2_PRINCE | M2_MALE | M2_COLLECT,
        M3_WAITFORU | M3_WANTSBOOK | M3_WANTSAMUL | M3_INFRAVISIBLE
            | M3_INFRAVISION,
        HI_LORD),
    MON("Geryon", "{{Geryon}}", S_DEMON, LVL(72, 3, -3, 75, 15),
        (G_HELL | G_NOCORPSE | G_NOGEN | G_UNIQ),
        A(ATTK(AT_CLAW, AD_PHYS, 3, 6), ATTK(AT_CLAW, AD_PHYS, 3, 6),
          ATTK(AT_STNG, AD_DRST, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1500, 500, MS_BRIBE, MZ_HUGE), MR_FIRE | MR_POISON, 0,
        M1_FLY | M1_SEE_INVIS | M1_POIS | M1_SLITHY,
        M2_NOPOLY | M2_DEMON | M2_STALK | M2_HOSTILE | M2_PNAME | M2_NASTY
            | M2_PRINCE | M2_MALE,
        M3_WANTSAMUL | M3_INFRAVISIBLE | M3_INFRAVISION, HI_LORD),
    MON("Dispater", "{{Dispater}}", S_DEMON, LVL(78, 15, -2, 80, 15),
        (G_HELL | G_NOCORPSE | G_NOGEN | G_UNIQ),
        A(ATTK(AT_WEAP, AD_PHYS, 4, 6), ATTK(AT_MAGC, AD_SPEL, 6, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1500, 500, MS_BRIBE, MZ_HUMAN), MR_FIRE | MR_POISON, 0,
        M1_FLY | M1_SEE_INVIS | M1_POIS | M1_HUMANOID,
        M2_NOPOLY | M2_DEMON | M2_STALK | M2_HOSTILE | M2_PNAME | M2_NASTY
            | M2_PRINCE | M2_MALE | M2_COLLECT,
        M3_WANTSAMUL | M3_INFRAVISIBLE | M3_INFRAVISION, HI_LORD),
    MON("Baalzebub", "{{Baalzebub}}", S_DEMON, LVL(89, 9, -5, 85, 20),
        (G_HELL | G_NOCORPSE | G_NOGEN | G_UNIQ),
        A(ATTK(AT_BITE, AD_DRST, 2, 6), ATTK(AT_GAZE, AD_STUN, 2, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1500, 500, MS_BRIBE, MZ_LARGE), MR_FIRE | MR_POISON, 0,
        M1_FLY | M1_SEE_INVIS | M1_POIS,
        M2_NOPOLY | M2_DEMON | M2_STALK | M2_HOSTILE | M2_PNAME | M2_NASTY
            | M2_PRINCE | M2_MALE,
        M3_WANTSAMUL | M3_WAITFORU | M3_INFRAVISIBLE | M3_INFRAVISION,
        HI_LORD),
    MON("Asmodeus", "{{Asmodeus}}", S_DEMON, LVL(105, 12, -7, 90, 20),
        (G_HELL | G_NOCORPSE | G_NOGEN | G_UNIQ),
        A(ATTK(AT_CLAW, AD_PHYS, 4, 4), ATTK(AT_MAGC, AD_COLD, 6, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1500, 500, MS_BRIBE, MZ_HUGE), MR_FIRE | MR_COLD | MR_POISON, 0,
        M1_FLY | M1_SEE_INVIS | M1_HUMANOID | M1_POIS,
        M2_NOPOLY | M2_DEMON | M2_STALK | M2_HOSTILE | M2_PNAME | M2_STRONG
            | M2_NASTY | M2_PRINCE | M2_MALE,
        M3_WANTSAMUL | M3_WAITFORU | M3_INFRAVISIBLE | M3_INFRAVISION,
        HI_LORD),
    MON("Demogorgon", "{{Demogorgon}}", S_DEMON, LVL(106, 15, -8, 95, -20),
        (G_HELL | G_NOCORPSE | G_NOGEN | G_UNIQ),
        A(ATTK(AT_MAGC, AD_SPEL, 8, 6), ATTK(AT_STNG, AD_DRLI, 1, 4),
          ATTK(AT_CLAW, AD_DISE, 1, 6), ATTK(AT_CLAW, AD_DISE, 1, 6), NO_ATTK,
          NO_ATTK),
        SIZ(1500, 500, MS_GROWL, MZ_HUGE), MR_FIRE | MR_POISON, 0,
        M1_FLY | M1_SEE_INVIS | M1_NOHANDS | M1_POIS,
        M2_NOPOLY | M2_DEMON | M2_STALK | M2_HOSTILE | M2_PNAME | M2_NASTY
            | M2_PRINCE | M2_MALE,
        M3_WANTSAMUL | M3_INFRAVISIBLE | M3_INFRAVISION, HI_LORD),
    /* Riders -- the Four Horsemen of the Apocalypse ("War" == player);
     * depicted with '&' but do not have M2_DEMON set.
     */
    MON("Death", "{{Death}}", S_DEMON, LVL(30, 12, -5, 100, 0), (G_UNIQ | G_NOGEN),
        A(ATTK(AT_TUCH, AD_DETH, 8, 8), ATTK(AT_TUCH, AD_DETH, 8, 8), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 1, MS_RIDER, MZ_HUMAN),
        MR_FIRE | MR_COLD | MR_ELEC | MR_SLEEP | MR_POISON | MR_STONE, 0,
        M1_FLY | M1_HUMANOID | M1_REGEN | M1_SEE_INVIS | M1_TPORT_CNTRL,
        M2_NOPOLY | M2_STALK | M2_HOSTILE | M2_PNAME | M2_STRONG | M2_NASTY,
        M3_INFRAVISIBLE | M3_INFRAVISION | M3_DISPLACES, HI_LORD),
    MON("Pestilence", "{{Pestilence}}", S_DEMON, LVL(30, 12, -5, 100, 0), (G_UNIQ | G_NOGEN),
        A(ATTK(AT_TUCH, AD_PEST, 8, 8), ATTK(AT_TUCH, AD_PEST, 8, 8), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 1, MS_RIDER, MZ_HUMAN),
        MR_FIRE | MR_COLD | MR_ELEC | MR_SLEEP | MR_POISON | MR_STONE, 0,
        M1_FLY | M1_HUMANOID | M1_REGEN | M1_SEE_INVIS | M1_TPORT_CNTRL,
        M2_NOPOLY | M2_STALK | M2_HOSTILE | M2_PNAME | M2_STRONG | M2_NASTY,
        M3_INFRAVISIBLE | M3_INFRAVISION | M3_DISPLACES, HI_LORD),
    MON("Famine", "{{Famine}}", S_DEMON, LVL(30, 12, -5, 100, 0), (G_UNIQ | G_NOGEN),
        A(ATTK(AT_TUCH, AD_FAMN, 8, 8), ATTK(AT_TUCH, AD_FAMN, 8, 8), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 1, MS_RIDER, MZ_HUMAN),
        MR_FIRE | MR_COLD | MR_ELEC | MR_SLEEP | MR_POISON | MR_STONE, 0,
        M1_FLY | M1_HUMANOID | M1_REGEN | M1_SEE_INVIS | M1_TPORT_CNTRL,
        M2_NOPOLY | M2_STALK | M2_HOSTILE | M2_PNAME | M2_STRONG | M2_NASTY,
        M3_INFRAVISIBLE | M3_INFRAVISION | M3_DISPLACES, HI_LORD),
    /* other demons
     */
#ifdef MAIL
    MON("mail daemon", "{{mail daemon}}", S_DEMON, LVL(56, 24, 10, 127, 0),
        (G_NOGEN | G_NOCORPSE),
        A(NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(600, 300, MS_SILENT, MZ_HUMAN),
        MR_FIRE | MR_COLD | MR_ELEC | MR_SLEEP | MR_POISON | MR_STONE, 0,
        M1_FLY | M1_SWIM | M1_BREATHLESS | M1_SEE_INVIS | M1_HUMANOID
            | M1_POIS,
        M2_NOPOLY | M2_STALK | M2_PEACEFUL, M3_INFRAVISIBLE | M3_INFRAVISION,
        CLR_BRIGHT_BLUE),
#endif
    MON("djinni", "{{djinni}}", S_DEMON, LVL(7, 12, 4, 30, 0), (G_NOGEN | G_NOCORPSE),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1500, 400, MS_DJINNI, MZ_HUMAN), MR_POISON | MR_STONE, 0,
        M1_HUMANOID | M1_FLY | M1_POIS, M2_NOPOLY | M2_STALK | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_YELLOW),
    /*
     * sea monsters
     */
    MON("jellyfish", "{{jellyfish}}", S_EEL, LVL(3, 3, 6, 0, 0), (G_GENO | G_NOGEN),
        A(ATTK(AT_STNG, AD_DRST, 3, 3), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(80, 20, MS_SILENT, MZ_SMALL), MR_POISON, MR_POISON,
        M1_SWIM | M1_AMPHIBIOUS | M1_SLITHY | M1_NOLIMBS | M1_NOHEAD
            | M1_NOTAKE | M1_POIS,
        M2_HOSTILE, 0, CLR_BLUE),
    MON("piranha", "{{piranha}}", S_EEL, LVL(5, 12, 4, 0, 0), (G_GENO | G_NOGEN | G_SGROUP),
        A(ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(60, 30, MS_SILENT, MZ_SMALL), 0, 0,
        M1_SWIM | M1_AMPHIBIOUS | M1_ANIMAL | M1_SLITHY | M1_NOLIMBS
            | M1_CARNIVORE | M1_OVIPAROUS | M1_NOTAKE,
        M2_HOSTILE, 0, CLR_RED),
    MON("shark", "{{shark}}", S_EEL, LVL(7, 12, 2, 0, 0), (G_GENO | G_NOGEN),
        A(ATTK(AT_BITE, AD_PHYS, 5, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(500, 350, MS_SILENT, MZ_LARGE), 0, 0,
        M1_SWIM | M1_AMPHIBIOUS | M1_ANIMAL | M1_SLITHY | M1_NOLIMBS
            | M1_CARNIVORE | M1_OVIPAROUS | M1_THICK_HIDE | M1_NOTAKE,
        M2_HOSTILE, 0, CLR_GRAY),
    MON("giant eel", "{{giant eel}}", S_EEL, LVL(5, 9, -1, 0, 0), (G_GENO | G_NOGEN),
        A(ATTK(AT_BITE, AD_PHYS, 3, 6), ATTK(AT_TUCH, AD_WRAP, 0, 0), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(200, 250, MS_SILENT, MZ_HUGE), 0, 0,
        M1_SWIM | M1_AMPHIBIOUS | M1_ANIMAL | M1_SLITHY | M1_NOLIMBS
            | M1_CARNIVORE | M1_OVIPAROUS | M1_NOTAKE,
        M2_HOSTILE, M3_INFRAVISIBLE, CLR_CYAN),
    MON("electric eel", "{{electric eel}}", S_EEL, LVL(7, 10, -3, 0, 0), (G_GENO | G_NOGEN),
        A(ATTK(AT_BITE, AD_ELEC, 4, 6), ATTK(AT_TUCH, AD_WRAP, 0, 0), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(200, 250, MS_SILENT, MZ_HUGE), MR_ELEC, MR_ELEC,
        M1_SWIM | M1_AMPHIBIOUS | M1_ANIMAL | M1_SLITHY | M1_NOLIMBS
            | M1_CARNIVORE | M1_OVIPAROUS | M1_NOTAKE,
        M2_HOSTILE, M3_INFRAVISIBLE, CLR_BRIGHT_BLUE),
    MON("kraken", "{{kraken}}", S_EEL, LVL(20, 3, 6, 0, -3), (G_GENO | G_NOGEN),
        A(ATTK(AT_CLAW, AD_PHYS, 2, 4), ATTK(AT_CLAW, AD_PHYS, 2, 4),
          ATTK(AT_HUGS, AD_WRAP, 2, 6), ATTK(AT_BITE, AD_PHYS, 5, 4), NO_ATTK,
          NO_ATTK),
        SIZ(1800, 1000, MS_SILENT, MZ_HUGE), 0, 0,
        M1_SWIM | M1_AMPHIBIOUS | M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE,
        M2_NOPOLY | M2_HOSTILE | M2_STRONG, M3_INFRAVISIBLE, CLR_RED),
    /*
     * lizards, &c
     */
    MON("newt", "{{newt}}", S_LIZARD, LVL(0, 6, 8, 0, 0), (G_GENO | 5),
        A(ATTK(AT_BITE, AD_PHYS, 1, 2), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(10, 20, MS_SILENT, MZ_TINY), 0, 0,
        M1_SWIM | M1_AMPHIBIOUS | M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE,
        M2_HOSTILE, 0, CLR_YELLOW),
    MON("gecko", "{{gecko}}", S_LIZARD, LVL(1, 6, 8, 0, 0), (G_GENO | 5),
        A(ATTK(AT_BITE, AD_PHYS, 1, 3), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(10, 20, MS_SQEEK, MZ_TINY), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, 0, CLR_GREEN),
    MON("iguana", "{{iguana}}", S_LIZARD, LVL(2, 6, 7, 0, 0), (G_GENO | 5),
        A(ATTK(AT_BITE, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(30, 30, MS_SILENT, MZ_TINY), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, 0, CLR_BROWN),
    MON("baby crocodile", "{{baby crocodile}}", S_LIZARD, LVL(3, 6, 7, 0, 0), G_GENO,
        A(ATTK(AT_BITE, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(200, 200, MS_SILENT, MZ_MEDIUM), 0, 0,
        M1_SWIM | M1_AMPHIBIOUS | M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE,
        M2_HOSTILE, 0, CLR_BROWN),
    MON("lizard", "{{lizard}}", S_LIZARD, LVL(5, 6, 6, 10, 0), (G_GENO | 5),
        A(ATTK(AT_BITE, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(10, 40, MS_SILENT, MZ_TINY), MR_STONE, MR_STONE,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, 0, CLR_GREEN),
    MON("chameleon", "{{chameleon}}", S_LIZARD, LVL(6, 5, 6, 10, 0), (G_GENO | 2),
        A(ATTK(AT_BITE, AD_PHYS, 4, 2), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(100, 100, MS_SILENT, MZ_TINY), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE,
        M2_NOPOLY | M2_HOSTILE | M2_SHAPESHIFTER, 0, CLR_BROWN),
    MON("crocodile", "{{crocodile}}", S_LIZARD, LVL(6, 9, 5, 0, 0), (G_GENO | 1),
        A(ATTK(AT_BITE, AD_PHYS, 4, 2), ATTK(AT_CLAW, AD_PHYS, 1, 12),
          NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_SILENT, MZ_LARGE), 0, 0,
        M1_SWIM | M1_AMPHIBIOUS | M1_ANIMAL | M1_THICK_HIDE | M1_NOHANDS
            | M1_OVIPAROUS | M1_CARNIVORE,
        M2_STRONG | M2_HOSTILE, 0, CLR_BROWN),
    MON("salamander", "{{salamander}}", S_LIZARD, LVL(8, 12, -1, 0, -9), (G_HELL | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 8), ATTK(AT_TUCH, AD_FIRE, 1, 6),
          ATTK(AT_HUGS, AD_PHYS, 2, 6), ATTK(AT_HUGS, AD_FIRE, 3, 6), NO_ATTK,
          NO_ATTK),
        SIZ(1500, 400, MS_MUMBLE, MZ_HUMAN), MR_SLEEP | MR_FIRE, MR_FIRE,
        M1_HUMANOID | M1_SLITHY | M1_THICK_HIDE | M1_POIS,
        M2_STALK | M2_HOSTILE | M2_COLLECT | M2_MAGIC, M3_INFRAVISIBLE,
        CLR_ORANGE),

    /*
     * dummy monster needed for visual interface
     * (marking it unique prevents figurines)
     */
    MON("long worm tail", "{{long worm tail}}", S_WORM_TAIL, LVL(0, 0, 0, 0, 0),
        (G_NOGEN | G_NOCORPSE | G_UNIQ),
        A(NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(0, 0, 0, 0), 0, 0, 0L, M2_NOPOLY, 0, CLR_BROWN),
    /* Note:
     * Worm tail must be between the normal monsters and the special
     * quest & pseudo-character ones because an optimization in the
     * random monster selection code assumes everything beyond here
     * has the G_NOGEN and M2_NOPOLY attributes.
     */

    /*
     * character classes
     */
    MON("archeologist", "{{archeologist}}", S_HUMAN, LVL(10, 12, 10, 1, 3), G_NOGEN,
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_TUNNEL | M1_NEEDPICK | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_COLLECT, M3_INFRAVISIBLE,
        HI_DOMESTIC),
    MON("barbarian", "{{barbarian}}", S_HUMAN, LVL(10, 12, 10, 1, 0), G_NOGEN,
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), MR_POISON, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_COLLECT, M3_INFRAVISIBLE,
        HI_DOMESTIC),
    MON("caveman", "{{caveman}}", S_HUMAN, LVL(10, 12, 10, 0, 1), G_NOGEN,
        A(ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_MALE | M2_COLLECT,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("cavewoman", "{{cavewoman}}",  S_HUMAN, LVL(10, 12, 10, 0, 1), G_NOGEN,
        A(ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_FEMALE | M2_COLLECT,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("healer", "{{healer}}", S_HUMAN, LVL(10, 12, 10, 1, 0), G_NOGEN,
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), MR_POISON, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_COLLECT, M3_INFRAVISIBLE,
        HI_DOMESTIC),
    MON("knight", "{{knight}}", S_HUMAN, LVL(10, 12, 10, 1, 3), G_NOGEN,
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_COLLECT, M3_INFRAVISIBLE,
        HI_DOMESTIC),
    MON("monk", "{{monk}}", S_HUMAN, LVL(10, 12, 10, 2, 0), G_NOGEN,
        A(ATTK(AT_CLAW, AD_PHYS, 1, 8), ATTK(AT_KICK, AD_PHYS, 1, 8), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_HERBIVORE,
        M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_COLLECT | M2_MALE,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("priest", "{{priest}}", S_HUMAN, LVL(10, 12, 10, 2, 0), G_NOGEN,
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_MALE | M2_COLLECT,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("priestess", "{{priestess}}", S_HUMAN, LVL(10, 12, 10, 2, 0), G_NOGEN,
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_FEMALE | M2_COLLECT,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("ranger", "{{ranger}}", S_HUMAN, LVL(10, 12, 10, 2, -3), G_NOGEN,
        A(ATTK(AT_WEAP, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_COLLECT, M3_INFRAVISIBLE,
        HI_DOMESTIC),
    MON("rogue", "{{rogue}}", S_HUMAN, LVL(10, 12, 10, 1, -3), G_NOGEN,
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_GREEDY | M2_JEWELS | M2_COLLECT,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("samurai", "{{samurai}}", S_HUMAN, LVL(10, 12, 10, 1, 3), G_NOGEN,
        A(ATTK(AT_WEAP, AD_PHYS, 1, 8), ATTK(AT_WEAP, AD_PHYS, 1, 8), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_COLLECT, M3_INFRAVISIBLE,
        HI_DOMESTIC),
    MON("tourist", "{{tourist}}", S_HUMAN, LVL(10, 12, 10, 1, 0), G_NOGEN,
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_COLLECT, M3_INFRAVISIBLE,
        HI_DOMESTIC),
    MON("valkyrie", "{{valkyrie}}", S_HUMAN, LVL(10, 12, 10, 1, -1), G_NOGEN,
        A(ATTK(AT_WEAP, AD_PHYS, 1, 8), ATTK(AT_WEAP, AD_PHYS, 1, 8), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), MR_COLD, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_FEMALE | M2_COLLECT,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("wizard", "{{wizard}}", S_HUMAN, LVL(10, 12, 10, 3, 0), G_NOGEN,
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_COLLECT | M2_MAGIC,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    /*
     * quest leaders
     */
    MON("Lord Carnarvon", "{{Lord Carnarvon}}", S_HUMAN, LVL(20, 12, 0, 30, 20), (G_NOGEN | G_UNIQ),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_LEADER, MZ_HUMAN), 0, 0,
        M1_TUNNEL | M1_NEEDPICK | M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PNAME | M2_PEACEFUL | M2_STRONG | M2_MALE
            | M2_COLLECT | M2_MAGIC,
        M3_CLOSE | M3_INFRAVISIBLE, HI_LORD),
    MON("Pelias", "{{Pelias}}", S_HUMAN, LVL(20, 12, 0, 30, 0), (G_NOGEN | G_UNIQ),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_LEADER, MZ_HUMAN), MR_POISON, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PNAME | M2_PEACEFUL | M2_STRONG | M2_MALE
            | M2_COLLECT | M2_MAGIC,
        M3_CLOSE | M3_INFRAVISIBLE, HI_LORD),
    MON("Shaman Karnov", "{{Shaman Karnov}}", S_HUMAN, LVL(20, 12, 0, 30, 20), (G_NOGEN | G_UNIQ),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_LEADER, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PNAME | M2_PEACEFUL | M2_STRONG | M2_MALE
            | M2_COLLECT | M2_MAGIC,
        M3_CLOSE | M3_INFRAVISIBLE, HI_LORD),
#if 0 /* OBSOLETE */
    /* Two for elves - one of each sex.
     */
    MON("Earendil", "{{Earendil}}", S_HUMAN,
        LVL(20, 12, 0, 50, -20), (G_NOGEN | G_UNIQ),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 8),
          NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_ELF, 350, MS_LEADER, MZ_HUMAN), MR_SLEEP, MR_SLEEP,
        M1_HUMANOID | M1_SEE_INVIS | M1_OMNIVORE,
        M2_NOPOLY | M2_ELF | M2_HUMAN | M2_PNAME | M2_PEACEFUL | M2_STRONG
          | M2_MALE | M2_COLLECT | M2_MAGIC,
        M3_CLOSE | M3_INFRAVISION | M3_INFRAVISIBLE, HI_LORD),
    MON("Elwing", "{{Elwing}}", S_HUMAN,
        LVL(20, 12, 0, 50, -20), (G_NOGEN | G_UNIQ),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 8),
          NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_ELF, 350, MS_LEADER, MZ_HUMAN), MR_SLEEP, MR_SLEEP,
        M1_HUMANOID | M1_SEE_INVIS | M1_OMNIVORE,
        M2_NOPOLY | M2_ELF | M2_HUMAN | M2_PNAME | M2_PEACEFUL | M2_STRONG
          | M2_FEMALE | M2_COLLECT | M2_MAGIC,
        M3_CLOSE | M3_INFRAVISION | M3_INFRAVISIBLE, HI_LORD),
#endif
    MON("Hippocrates", "{{Hippocrates}}", S_HUMAN, LVL(20, 12, 0, 40, 0), (G_NOGEN | G_UNIQ),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_LEADER, MZ_HUMAN), MR_POISON, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PNAME | M2_PEACEFUL | M2_STRONG | M2_MALE
            | M2_COLLECT | M2_MAGIC,
        M3_CLOSE | M3_INFRAVISIBLE, HI_LORD),
    MON("King Arthur", "{{King Arthur}}", S_HUMAN, LVL(20, 12, 0, 40, 20), (G_NOGEN | G_UNIQ),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_LEADER, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PNAME | M2_PEACEFUL | M2_STRONG | M2_MALE
            | M2_COLLECT | M2_MAGIC,
        M3_CLOSE | M3_INFRAVISIBLE, HI_LORD),
    MON("Grand Master", "{{Grand Master}}", S_HUMAN, LVL(25, 12, 0, 70, 0), (G_NOGEN | G_UNIQ),
        A(ATTK(AT_CLAW, AD_PHYS, 4, 10), ATTK(AT_KICK, AD_PHYS, 2, 8),
          ATTK(AT_MAGC, AD_CLRC, 2, 8), ATTK(AT_MAGC, AD_CLRC, 2, 8), NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_LEADER, MZ_HUMAN),
        MR_FIRE | MR_ELEC | MR_SLEEP | MR_POISON, 0,
        M1_HUMANOID | M1_SEE_INVIS | M1_HERBIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_MALE | M2_NASTY
            | M2_MAGIC,
        M3_CLOSE | M3_INFRAVISIBLE, CLR_BLACK),
    MON("Arch Priest", "{{Arch Priest}}", S_HUMAN, LVL(25, 12, 7, 70, 0), (G_NOGEN | G_UNIQ),
        A(ATTK(AT_WEAP, AD_PHYS, 4, 10), ATTK(AT_KICK, AD_PHYS, 2, 8),
          ATTK(AT_MAGC, AD_CLRC, 2, 8), ATTK(AT_MAGC, AD_CLRC, 2, 8), NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_LEADER, MZ_HUMAN),
        MR_FIRE | MR_ELEC | MR_SLEEP | MR_POISON, 0,
        M1_HUMANOID | M1_SEE_INVIS | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_MALE | M2_COLLECT
            | M2_MAGIC,
        M3_CLOSE | M3_INFRAVISIBLE, CLR_WHITE),
    MON("Orion", "{{Orion}}", S_HUMAN, LVL(20, 12, 0, 30, 0), (G_NOGEN | G_UNIQ),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(2200, 700, MS_LEADER, MZ_HUGE), 0, 0,
        M1_HUMANOID | M1_OMNIVORE | M1_SEE_INVIS | M1_SWIM | M1_AMPHIBIOUS,
        M2_NOPOLY | M2_HUMAN | M2_PNAME | M2_PEACEFUL | M2_STRONG | M2_MALE
            | M2_COLLECT | M2_MAGIC,
        M3_CLOSE | M3_INFRAVISION | M3_INFRAVISIBLE, HI_LORD),
    /* Note: Master of Thieves is also the Tourist's nemesis.
     */
    MON("Master of Thieves", "{{Master of Thieves}}", S_HUMAN, LVL(20, 12, 0, 30, -20),
        (G_NOGEN | G_UNIQ),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 6), ATTK(AT_WEAP, AD_PHYS, 2, 6),
          ATTK(AT_CLAW, AD_SAMU, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_LEADER, MZ_HUMAN), MR_STONE, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_MALE | M2_GREEDY
            | M2_JEWELS | M2_COLLECT | M2_MAGIC,
        M3_CLOSE | M3_INFRAVISIBLE, HI_LORD),
    MON("Lord Sato", "{{Lord Sato}}", S_HUMAN, LVL(20, 12, 0, 30, 20), (G_NOGEN | G_UNIQ),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 8), ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_LEADER, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PNAME | M2_PEACEFUL | M2_STRONG | M2_MALE
            | M2_COLLECT | M2_MAGIC,
        M3_CLOSE | M3_INFRAVISIBLE, HI_LORD),
    MON("Twoflower", "{{Twoflower}}", S_HUMAN, LVL(20, 12, 10, 20, 0), (G_NOGEN | G_UNIQ),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_LEADER, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PNAME | M2_PEACEFUL | M2_STRONG | M2_MALE
            | M2_COLLECT | M2_MAGIC,
        M3_CLOSE | M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("Norn", "{{Norn}}", S_HUMAN, LVL(20, 12, 0, 80, 0), (G_NOGEN | G_UNIQ),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 8), ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1800, 550, MS_LEADER, MZ_HUGE), MR_COLD, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_FEMALE
            | M2_COLLECT | M2_MAGIC,
        M3_CLOSE | M3_INFRAVISIBLE, HI_LORD),
    MON("Neferet the Green", "{{Neferet the Green}}", S_HUMAN, LVL(20, 12, 0, 60, 0),
        (G_NOGEN | G_UNIQ),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_MAGC, AD_SPEL, 2, 8), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_LEADER, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_FEMALE | M2_PNAME | M2_PEACEFUL | M2_STRONG
            | M2_COLLECT | M2_MAGIC,
        M3_CLOSE | M3_INFRAVISIBLE, CLR_GREEN),
    /*
     * quest nemeses
     */
    MON("Minion of Huhetotl", "{{Minion of Huhetotl}}", S_DEMON, LVL(16, 12, -2, 75, -14),
        (G_NOCORPSE | G_NOGEN | G_UNIQ),
        A(ATTK(AT_WEAP, AD_PHYS, 8, 4), ATTK(AT_WEAP, AD_PHYS, 4, 6),
          ATTK(AT_MAGC, AD_SPEL, 0, 0), ATTK(AT_CLAW, AD_SAMU, 2, 6), NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_NEMESIS, MZ_LARGE),
        MR_FIRE | MR_POISON | MR_STONE, 0, M1_FLY | M1_SEE_INVIS | M1_POIS,
        M2_NOPOLY | M2_DEMON | M2_STALK | M2_HOSTILE | M2_STRONG | M2_NASTY
            | M2_COLLECT,
        M3_WANTSARTI | M3_WAITFORU | M3_INFRAVISION | M3_INFRAVISIBLE,
        CLR_RED),
    MON("Thoth Amon", "{{Thoth Amon}}", S_HUMAN, LVL(16, 12, 0, 10, -14),
        (G_NOGEN | G_UNIQ | G_NOCORPSE),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_MAGC, AD_SPEL, 0, 0),
          ATTK(AT_MAGC, AD_SPEL, 0, 0), ATTK(AT_CLAW, AD_SAMU, 1, 4), NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_NEMESIS, MZ_HUMAN), MR_POISON | MR_STONE, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PNAME | M2_STRONG | M2_MALE | M2_STALK
            | M2_HOSTILE | M2_NASTY | M2_COLLECT | M2_MAGIC,
        M3_WANTSARTI | M3_WAITFORU | M3_INFRAVISIBLE, HI_LORD),
    /* Multi-headed, possessing the breath attacks of all the other dragons
     * (selected at random when attacking).
     */
    MON("Chromatic Dragon", "{{Chromatic Dragon}}", S_DRAGON, LVL(16, 12, 0, 30, -14),
        (G_NOGEN | G_UNIQ),
        A(ATTK(AT_BREA, AD_RBRE, 6, 6), ATTK(AT_MAGC, AD_SPEL, 0, 0),
          ATTK(AT_CLAW, AD_SAMU, 2, 8), ATTK(AT_BITE, AD_PHYS, 4, 8),
          ATTK(AT_BITE, AD_PHYS, 4, 8), ATTK(AT_STNG, AD_PHYS, 1, 6)),
        SIZ(WT_DRAGON, 1700, MS_NEMESIS, MZ_GIGANTIC),
        MR_FIRE | MR_COLD | MR_SLEEP | MR_DISINT | MR_ELEC | MR_POISON
            | MR_ACID | MR_STONE,
        MR_FIRE | MR_COLD | MR_SLEEP | MR_DISINT | MR_ELEC | MR_POISON
            | MR_STONE,
        M1_THICK_HIDE | M1_NOHANDS | M1_CARNIVORE | M1_SEE_INVIS | M1_POIS,
        M2_NOPOLY | M2_HOSTILE | M2_FEMALE | M2_STALK | M2_STRONG | M2_NASTY
            | M2_GREEDY | M2_JEWELS | M2_MAGIC,
        M3_WANTSARTI | M3_WAITFORU | M3_INFRAVISIBLE, HI_LORD),
#if 0 /* OBSOLETE */
    MON("Goblin King", "{{Goblin King}}", S_ORC,
        LVL(15, 12, 10, 0, -15), (G_NOGEN | G_UNIQ),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 6), ATTK(AT_WEAP, AD_PHYS, 2, 6),
          ATTK(AT_CLAW, AD_SAMU, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(750, 350, MS_NEMESIS, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_ORC | M2_HOSTILE | M2_STRONG | M2_STALK | M2_NASTY
          | M2_MALE | M2_GREEDY | M2_JEWELS | M2_COLLECT | M2_MAGIC,
        M3_WANTSARTI | M3_WAITFORU | M3_INFRAVISION | M3_INFRAVISIBLE,
        HI_LORD),
#endif
    MON("Cyclops", "{{Cyclops}}", S_GIANT, LVL(18, 12, 0, 0, -15), (G_NOGEN | G_UNIQ),
        A(ATTK(AT_WEAP, AD_PHYS, 4, 8), ATTK(AT_WEAP, AD_PHYS, 4, 8),
          ATTK(AT_CLAW, AD_SAMU, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1900, 700, MS_NEMESIS, MZ_HUGE), MR_STONE, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_GIANT | M2_STRONG | M2_ROCKTHROW | M2_STALK
            | M2_HOSTILE | M2_NASTY | M2_MALE | M2_JEWELS | M2_COLLECT,
        M3_WANTSARTI | M3_WAITFORU | M3_INFRAVISION | M3_INFRAVISIBLE,
        CLR_GRAY),
    MON("Ixoth", "{{Ixoth}}", S_DRAGON, LVL(15, 12, -1, 20, -14), (G_NOGEN | G_UNIQ),
        A(ATTK(AT_BREA, AD_FIRE, 8, 6), ATTK(AT_BITE, AD_PHYS, 4, 8),
          ATTK(AT_MAGC, AD_SPEL, 0, 0), ATTK(AT_CLAW, AD_PHYS, 2, 4),
          ATTK(AT_CLAW, AD_SAMU, 2, 4), NO_ATTK),
        SIZ(WT_DRAGON, 1600, MS_NEMESIS, MZ_GIGANTIC), MR_FIRE | MR_STONE,
        MR_FIRE,
        M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_CARNIVORE | M1_SEE_INVIS,
        M2_NOPOLY | M2_MALE | M2_PNAME | M2_HOSTILE | M2_STRONG | M2_NASTY
            | M2_STALK | M2_GREEDY | M2_JEWELS | M2_MAGIC,
        M3_WANTSARTI | M3_WAITFORU | M3_INFRAVISIBLE, CLR_RED),
    MON("Master Kaen", "{{Master Kaen}}", S_HUMAN, LVL(25, 12, -10, 10, -20), (G_NOGEN | G_UNIQ),
        A(ATTK(AT_CLAW, AD_PHYS, 16, 2), ATTK(AT_CLAW, AD_PHYS, 16, 2),
          ATTK(AT_MAGC, AD_CLRC, 0, 0), ATTK(AT_CLAW, AD_SAMU, 1, 4), NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_NEMESIS, MZ_HUMAN), MR_POISON | MR_STONE,
        MR_POISON, M1_HUMANOID | M1_HERBIVORE | M1_SEE_INVIS,
        M2_NOPOLY | M2_HUMAN | M2_MALE | M2_PNAME | M2_HOSTILE | M2_STRONG
            | M2_NASTY | M2_STALK | M2_COLLECT | M2_MAGIC,
        M3_WANTSARTI | M3_WAITFORU | M3_INFRAVISIBLE, HI_LORD),
    MON("Nalzok", "{{Nalzok}}", S_DEMON, LVL(16, 12, -2, 85, -127),
        (G_NOGEN | G_UNIQ | G_NOCORPSE),
        A(ATTK(AT_WEAP, AD_PHYS, 8, 4), ATTK(AT_WEAP, AD_PHYS, 4, 6),
          ATTK(AT_MAGC, AD_SPEL, 0, 0), ATTK(AT_CLAW, AD_SAMU, 2, 6), NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_NEMESIS, MZ_LARGE),
        MR_FIRE | MR_POISON | MR_STONE, 0, M1_FLY | M1_SEE_INVIS | M1_POIS,
        M2_NOPOLY | M2_DEMON | M2_MALE | M2_PNAME | M2_HOSTILE | M2_STRONG
            | M2_STALK | M2_NASTY | M2_COLLECT,
        M3_WANTSARTI | M3_WAITFORU | M3_INFRAVISION | M3_INFRAVISIBLE,
        CLR_RED),
    MON("Scorpius", "{{Scorpius}}", S_SPIDER, LVL(15, 12, 10, 0, -15), (G_NOGEN | G_UNIQ),
        A(ATTK(AT_CLAW, AD_PHYS, 2, 6), ATTK(AT_CLAW, AD_SAMU, 2, 6),
          ATTK(AT_STNG, AD_DISE, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(750, 350, MS_NEMESIS, MZ_HUMAN), MR_POISON | MR_STONE, MR_POISON,
        M1_ANIMAL | M1_NOHANDS | M1_OVIPAROUS | M1_POIS | M1_CARNIVORE,
        M2_NOPOLY | M2_MALE | M2_PNAME | M2_HOSTILE | M2_STRONG | M2_STALK
            | M2_NASTY | M2_COLLECT | M2_MAGIC,
        M3_WANTSARTI | M3_WAITFORU, HI_LORD),
    MON("Master Assassin", "{{Master Assassin}}", S_HUMAN, LVL(15, 12, 0, 30, 18),
        (G_NOGEN | G_UNIQ),
        A(ATTK(AT_WEAP, AD_DRST, 2, 6), ATTK(AT_WEAP, AD_PHYS, 2, 8),
          ATTK(AT_CLAW, AD_SAMU, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_NEMESIS, MZ_HUMAN), MR_STONE, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_MALE | M2_HOSTILE | M2_STALK
            | M2_NASTY | M2_COLLECT | M2_MAGIC,
        M3_WANTSARTI | M3_WAITFORU | M3_INFRAVISIBLE, HI_LORD),
    /* A renegade daimyo who led a 13 year civil war against the shogun
     * of his time.
     */
    MON("Ashikaga Takauji", "{{Ashikaga Takauji}}", S_HUMAN, LVL(15, 12, 0, 40, -13),
        (G_NOGEN | G_UNIQ | G_NOCORPSE),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 6), ATTK(AT_WEAP, AD_PHYS, 2, 6),
          ATTK(AT_CLAW, AD_SAMU, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_NEMESIS, MZ_HUMAN), MR_STONE, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PNAME | M2_HOSTILE | M2_STRONG | M2_STALK
            | M2_NASTY | M2_MALE | M2_COLLECT | M2_MAGIC,
        M3_WANTSARTI | M3_WAITFORU | M3_INFRAVISIBLE, HI_LORD),
    /*
     * Note: the Master of Thieves was defined above.
     */
    MON("Lord Surtur", "{{Lord Surtur}}", S_GIANT, LVL(15, 12, 2, 50, 12), (G_NOGEN | G_UNIQ),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 10), ATTK(AT_WEAP, AD_PHYS, 2, 10),
          ATTK(AT_CLAW, AD_SAMU, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(2250, 850, MS_NEMESIS, MZ_HUGE), MR_FIRE | MR_STONE, MR_FIRE,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_GIANT | M2_MALE | M2_PNAME | M2_HOSTILE | M2_STALK
            | M2_STRONG | M2_NASTY | M2_ROCKTHROW | M2_JEWELS | M2_COLLECT,
        M3_WANTSARTI | M3_WAITFORU | M3_INFRAVISION | M3_INFRAVISIBLE,
        HI_LORD),
    MON("Dark One", "{{Dark One}}", S_HUMAN, LVL(15, 12, 0, 80, -10),
        (G_NOGEN | G_UNIQ | G_NOCORPSE),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_WEAP, AD_PHYS, 1, 6),
          ATTK(AT_CLAW, AD_SAMU, 1, 4), ATTK(AT_MAGC, AD_SPEL, 0, 0), NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_NEMESIS, MZ_HUMAN), MR_STONE, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_HOSTILE | M2_STALK | M2_NASTY
            | M2_COLLECT | M2_MAGIC,
        M3_WANTSARTI | M3_WAITFORU | M3_INFRAVISIBLE, CLR_BLACK),
    /*
     * quest "guardians"
     */
    MON("student", "{{student}}", S_HUMAN, LVL(5, 12, 10, 10, 3), G_NOGEN,
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_GUARDIAN, MZ_HUMAN), 0, 0,
        M1_TUNNEL | M1_NEEDPICK | M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_COLLECT,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("chieftain", "{{chieftain}}", S_HUMAN, LVL(5, 12, 10, 10, 0), G_NOGEN,
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_GUARDIAN, MZ_HUMAN), MR_POISON, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_COLLECT,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("neanderthal", "{{neanderthal}}", S_HUMAN, LVL(5, 12, 10, 10, 1), G_NOGEN,
        A(ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_GUARDIAN, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_COLLECT,
        M3_INFRAVISIBLE, HI_DOMESTIC),
#if 0 /* OBSOLETE */
    MON("High-elf", "{{High-elf}}", S_HUMAN,
        LVL(5, 12, 10, 10, -7), G_NOGEN,
        A(ATTK(AT_WEAP, AD_PHYS, 2, 4), ATTK(AT_MAGC, AD_CLRC, 0, 0),
          NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_ELF, 350, MS_GUARDIAN, MZ_HUMAN), MR_SLEEP, MR_SLEEP,
        M1_HUMANOID | M1_SEE_INVIS | M1_OMNIVORE,
        M2_NOPOLY | M2_ELF | M2_PEACEFUL | M2_COLLECT,
        M3_INFRAVISION | M3_INFRAVISIBLE, HI_DOMESTIC),
#endif
    MON("attendant", "{{attendant}}", S_HUMAN, LVL(5, 12, 10, 10, 3), G_NOGEN,
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_GUARDIAN, MZ_HUMAN), MR_POISON, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_COLLECT,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("page", "{{page}}", S_HUMAN, LVL(5, 12, 10, 10, 3), G_NOGEN,
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_GUARDIAN, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_COLLECT,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("abbot", "{{abbot}}", S_HUMAN, LVL(5, 12, 10, 20, 0), G_NOGEN,
        A(ATTK(AT_CLAW, AD_PHYS, 8, 2), ATTK(AT_KICK, AD_STUN, 3, 2),
          ATTK(AT_MAGC, AD_CLRC, 0, 0), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_GUARDIAN, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_HERBIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_COLLECT,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("acolyte", "{{acolyte}}", S_HUMAN, LVL(5, 12, 10, 20, 0), G_NOGEN,
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_MAGC, AD_CLRC, 0, 0), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_GUARDIAN, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_COLLECT,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("hunter", "{{hunter}}", S_HUMAN, LVL(5, 12, 10, 10, -7), G_NOGEN,
        A(ATTK(AT_WEAP, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_GUARDIAN, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_SEE_INVIS | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_COLLECT,
        M3_INFRAVISION | M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("thug", "{{thug}}", S_HUMAN, LVL(5, 12, 10, 10, -3), G_NOGEN,
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_GUARDIAN, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE, M2_NOPOLY | M2_HUMAN | M2_PEACEFUL
                                       | M2_STRONG | M2_GREEDY | M2_COLLECT,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("ninja", "{{ninja}}", S_HUMAN, LVL(5, 12, 10, 10, 3), G_NOGEN,
        A(ATTK(AT_WEAP, AD_PHYS, 1, 8), ATTK(AT_WEAP, AD_PHYS, 1, 8), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_HOSTILE | M2_STRONG | M2_COLLECT,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("roshi", "{{roshi}}", S_HUMAN, LVL(5, 12, 10, 10, 3), G_NOGEN,
        A(ATTK(AT_WEAP, AD_PHYS, 1, 8), ATTK(AT_WEAP, AD_PHYS, 1, 8), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_GUARDIAN, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_COLLECT,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("guide", "{{guide}}", S_HUMAN, LVL(5, 12, 10, 20, 0), G_NOGEN,
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_MAGC, AD_SPEL, 0, 0), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_GUARDIAN, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE, M2_NOPOLY | M2_HUMAN | M2_PEACEFUL
                                       | M2_STRONG | M2_COLLECT | M2_MAGIC,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("warrior", "{{warrior}}", S_HUMAN, LVL(5, 12, 10, 10, -1), G_NOGEN,
        A(ATTK(AT_WEAP, AD_PHYS, 1, 8), ATTK(AT_WEAP, AD_PHYS, 1, 8), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_GUARDIAN, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE, M2_NOPOLY | M2_HUMAN | M2_PEACEFUL
                                       | M2_STRONG | M2_COLLECT | M2_FEMALE,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("apprentice", "{{apprentice}}", S_HUMAN, LVL(5, 12, 10, 30, 0), G_NOGEN,
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_MAGC, AD_SPEL, 0, 0), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_GUARDIAN, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE, M2_NOPOLY | M2_HUMAN | M2_PEACEFUL
                                       | M2_STRONG | M2_COLLECT | M2_MAGIC,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    /*
     * array terminator
     */
    MON("", "{{}}", 0, LVL(0, 0, 0, 0, 0), (0),
        A(NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(0, 0, 0, 0), 0, 0, 0L, 0L, 0, 0)
};
#endif /* !SPLITMON_1 */
// <<< CN_TS

#ifndef SPLITMON_1
/* dummy routine used to force linkage */
void
monst_init()
{
    return;
}

struct attack sa_yes[NATTK] = SEDUCTION_ATTACKS_YES;
struct attack sa_no[NATTK] = SEDUCTION_ATTACKS_NO;
#endif

/*monst.c*/
