/* NetHack 3.6	detect.c	$NHDT-Date: 1522891623 2018/04/05 01:27:03 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.81 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2018. */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * Detection routines, including crystal ball, magic mapping, and search
 * command.
 */

/* JNetHack Copyright */
/* (c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994-2000  */
/* For 3.4-, Copyright (c) SHIRAKATA Kentaro, 2002-2018            */
/* JNetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "artifact.h"

extern boolean known; /* from read.c */

STATIC_DCL boolean NDECL(unconstrain_map);
STATIC_DCL void NDECL(reconstrain_map);
STATIC_DCL void FDECL(browse_map, (int, const char *));
STATIC_DCL void FDECL(map_monst, (struct monst *, BOOLEAN_P));
STATIC_DCL void FDECL(do_dknown_of, (struct obj *));
STATIC_DCL boolean FDECL(check_map_spot, (int, int, CHAR_P, unsigned));
STATIC_DCL boolean FDECL(clear_stale_map, (CHAR_P, unsigned));
STATIC_DCL void FDECL(sense_trap, (struct trap *, XCHAR_P, XCHAR_P, int));
STATIC_DCL int FDECL(detect_obj_traps, (struct obj *, BOOLEAN_P, int));
STATIC_DCL void FDECL(show_map_spot, (int, int));
STATIC_PTR void FDECL(findone, (int, int, genericptr_t));
STATIC_PTR void FDECL(openone, (int, int, genericptr_t));
STATIC_DCL int FDECL(mfind0, (struct monst *, BOOLEAN_P));
STATIC_DCL int FDECL(reveal_terrain_getglyph, (int, int, int,
                                               unsigned, int, int));

/* bring hero out from underwater or underground or being engulfed;
   return True iff any change occurred */
STATIC_OVL boolean
unconstrain_map()
{
    boolean res = u.uinwater || u.uburied || u.uswallow;

    /* bring Underwater, buried, or swallowed hero to normal map */
    iflags.save_uinwater = u.uinwater, u.uinwater = 0;
    iflags.save_uburied  = u.uburied,  u.uburied  = 0;
    iflags.save_uswallow = u.uswallow, u.uswallow = 0;

    return res;
}

/* put hero back underwater or underground or engulfed */
STATIC_OVL void
reconstrain_map()
{
    u.uinwater = iflags.save_uinwater, iflags.save_uinwater = 0;
    u.uburied  = iflags.save_uburied,  iflags.save_uburied  = 0;
    u.uswallow = iflags.save_uswallow, iflags.save_uswallow = 0;
}

/* use getpos()'s 'autodescribe' to view whatever is currently shown on map */
STATIC_DCL void
browse_map(ter_typ, ter_explain)
int ter_typ;
const char *ter_explain;
{
    coord dummy_pos; /* don't care whether player actually picks a spot */
    boolean save_autodescribe;

    dummy_pos.x = u.ux, dummy_pos.y = u.uy; /* starting spot for getpos() */
    save_autodescribe = iflags.autodescribe;
    iflags.autodescribe = TRUE;
    iflags.terrainmode = ter_typ;
    getpos(&dummy_pos, FALSE, ter_explain);
    iflags.terrainmode = 0;
    iflags.autodescribe = save_autodescribe;
}

/* extracted from monster_detection() so can be shared by do_vicinity_map() */
STATIC_DCL void
map_monst(mtmp, showtail)
struct monst *mtmp;
boolean showtail;
{
    if (def_monsyms[(int) mtmp->data->mlet].sym == ' ')
        show_glyph(mtmp->mx, mtmp->my, detected_mon_to_glyph(mtmp));
    else
        show_glyph(mtmp->mx, mtmp->my,
                   mtmp->mtame ? pet_to_glyph(mtmp) : mon_to_glyph(mtmp));

    if (showtail && mtmp->data == &mons[PM_LONG_WORM])
        detect_wsegs(mtmp, 0);
}

/* this is checking whether a trap symbol represents a trapped chest,
   not whether a trapped chest is actually present */
boolean
trapped_chest_at(ttyp, x, y)
int ttyp;
int x, y;
{
    struct monst *mtmp;
    struct obj *otmp;

    if (!glyph_is_trap(glyph_at(x, y)))
        return FALSE;
    if (ttyp != BEAR_TRAP || (Hallucination && rn2(20)))
        return FALSE;

    /*
     * TODO?  We should check containers recursively like the trap
     * detecting routine does.  Chests and large boxes do not nest in
     * themselves or each other, but could be contained inside statues.
     *
     * For farlook, we should also check for buried containers, but
     * for '^' command to examine adjacent trap glyph, we shouldn't.
     */

    /* on map, presence of any trappable container will do */
    if (sobj_at(CHEST, x, y) || sobj_at(LARGE_BOX, x, y))
        return TRUE;
    /* in inventory, we need to find one which is actually trapped */
    if (x == u.ux && y == u.uy) {
        for (otmp = invent; otmp; otmp = otmp->nobj)
            if (Is_box(otmp) && otmp->otrapped)
                return TRUE;
        if (u.usteed) { /* steed isn't on map so won't be found by m_at() */
            for (otmp = u.usteed->minvent; otmp; otmp = otmp->nobj)
                if (Is_box(otmp) && otmp->otrapped)
                    return TRUE;
        }
    }
    if ((mtmp = m_at(x, y)) != 0)
        for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
            if (Is_box(otmp) && otmp->otrapped)
                return TRUE;
    return FALSE;
}

/* this is checking whether a trap symbol represents a trapped door,
   not whether the door here is actually trapped */
boolean
trapped_door_at(ttyp, x, y)
int ttyp;
int x, y;
{
    struct rm *lev;

    if (!glyph_is_trap(glyph_at(x, y)))
        return FALSE;
    if (ttyp != BEAR_TRAP || (Hallucination && rn2(20)))
        return FALSE;
    lev = &levl[x][y];
    if (!IS_DOOR(lev->typ))
        return FALSE;
    if ((lev->doormask & (D_NODOOR | D_BROKEN | D_ISOPEN)) != 0
         && trapped_chest_at(ttyp, x, y))
        return FALSE;
    return TRUE;
}

/* recursively search obj for an object in class oclass, return 1st found */
struct obj *
o_in(obj, oclass)
struct obj *obj;
char oclass;
{
    register struct obj *otmp;
    struct obj *temp;

    if (obj->oclass == oclass)
        return obj;

    if (Has_contents(obj)) {
        for (otmp = obj->cobj; otmp; otmp = otmp->nobj)
            if (otmp->oclass == oclass)
                return otmp;
            else if (Has_contents(otmp) && (temp = o_in(otmp, oclass)) != 0)
                return temp;
    }
    return (struct obj *) 0;
}

/* Recursively search obj for an object made of specified material.
 * Return first found.
 */
struct obj *
o_material(obj, material)
struct obj *obj;
unsigned material;
{
    register struct obj *otmp;
    struct obj *temp;

    if (objects[obj->otyp].oc_material == material)
        return obj;

    if (Has_contents(obj)) {
        for (otmp = obj->cobj; otmp; otmp = otmp->nobj)
            if (objects[otmp->otyp].oc_material == material)
                return otmp;
            else if (Has_contents(otmp)
                     && (temp = o_material(otmp, material)) != 0)
                return temp;
    }
    return (struct obj *) 0;
}

STATIC_OVL void
do_dknown_of(obj)
struct obj *obj;
{
    struct obj *otmp;

    obj->dknown = 1;
    if (Has_contents(obj)) {
        for (otmp = obj->cobj; otmp; otmp = otmp->nobj)
            do_dknown_of(otmp);
    }
}

/* Check whether the location has an outdated object displayed on it. */
STATIC_OVL boolean
check_map_spot(x, y, oclass, material)
int x, y;
char oclass;
unsigned material;
{
    int glyph;
    register struct obj *otmp;
    register struct monst *mtmp;

    glyph = glyph_at(x, y);
    if (glyph_is_object(glyph)) {
        /* there's some object shown here */
        if (oclass == ALL_CLASSES) {
            return (boolean) !(level.objects[x][y] /* stale if nothing here */
                               || ((mtmp = m_at(x, y)) != 0 && mtmp->minvent));
        } else {
            if (material
                && objects[glyph_to_obj(glyph)].oc_material == material) {
                /* object shown here is of interest because material matches */
                for (otmp = level.objects[x][y]; otmp; otmp = otmp->nexthere)
                    if (o_material(otmp, GOLD))
                        return FALSE;
                /* didn't find it; perhaps a monster is carrying it */
                if ((mtmp = m_at(x, y)) != 0) {
                    for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
                        if (o_material(otmp, GOLD))
                            return FALSE;
                }
                /* detection indicates removal of this object from the map */
                return TRUE;
            }
            if (oclass && objects[glyph_to_obj(glyph)].oc_class == oclass) {
                /* obj shown here is of interest because its class matches */
                for (otmp = level.objects[x][y]; otmp; otmp = otmp->nexthere)
                    if (o_in(otmp, oclass))
                        return FALSE;
                /* didn't find it; perhaps a monster is carrying it */
                if ((mtmp = m_at(x, y)) != 0) {
                    for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
                        if (o_in(otmp, oclass))
                            return FALSE;
                }
                /* detection indicates removal of this object from the map */
                return TRUE;
            }
        }
    }
    return FALSE;
}

/*
 * When doing detection, remove stale data from the map display (corpses
 * rotted away, objects carried away by monsters, etc) so that it won't
 * reappear after the detection has completed.  Return true if noticeable
 * change occurs.
 */
STATIC_OVL boolean
clear_stale_map(oclass, material)
char oclass;
unsigned material;
{
    register int zx, zy;
    boolean change_made = FALSE;

    for (zx = 1; zx < COLNO; zx++)
        for (zy = 0; zy < ROWNO; zy++)
            if (check_map_spot(zx, zy, oclass, material)) {
                unmap_object(zx, zy);
                change_made = TRUE;
            }

    return change_made;
}

/* look for gold, on the floor or in monsters' possession */
int
gold_detect(sobj)
register struct obj *sobj;
{
    register struct obj *obj;
    register struct monst *mtmp;
    struct obj gold, *temp = 0;
    boolean stale, ugold = FALSE, steedgold = FALSE;
    int ter_typ = TER_DETECT | TER_OBJ;

    known = stale = clear_stale_map(COIN_CLASS,
                                    (unsigned) (sobj->blessed ? GOLD : 0));

    /* look for gold carried by monsters (might be in a container) */
    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp))
            continue; /* probably not needed in this case but... */
        if (findgold(mtmp->minvent) || monsndx(mtmp->data) == PM_GOLD_GOLEM) {
            if (mtmp == u.usteed) {
                steedgold = TRUE;
            } else {
                known = TRUE;
                goto outgoldmap; /* skip further searching */
            }
        } else {
            for (obj = mtmp->minvent; obj; obj = obj->nobj)
                if ((sobj->blessed && o_material(obj, GOLD))
                    || o_in(obj, COIN_CLASS)) {
                    if (mtmp == u.usteed) {
                        steedgold = TRUE;
                    } else {
                        known = TRUE;
                        goto outgoldmap; /* skip further searching */
                    }
                }
        }
    }

    /* look for gold objects */
    for (obj = fobj; obj; obj = obj->nobj) {
        if (sobj->blessed && o_material(obj, GOLD)) {
            known = TRUE;
            if (obj->ox != u.ux || obj->oy != u.uy)
                goto outgoldmap;
        } else if (o_in(obj, COIN_CLASS)) {
            known = TRUE;
            if (obj->ox != u.ux || obj->oy != u.uy)
                goto outgoldmap;
        }
    }

    if (!known) {
        /* no gold found on floor or monster's inventory.
           adjust message if you have gold in your inventory */
        if (sobj) {
            char buf[BUFSZ];

            if (youmonst.data == &mons[PM_GOLD_GOLEM])
/*JP
                Sprintf(buf, "You feel like a million %s!", currency(2L));
*/
                Strcpy(buf, "���Ȃ��͋������ɂȂ����悤�Ɋ������I");
            else if (money_cnt(invent) || hidden_gold())
                Strcpy(buf,
/*JP
                   "You feel worried about your future financial situation.");
*/
                   "���Ȃ��͏����̌o�Ϗ󋵂��S�z�ɂȂ����D");
            else if (steedgold)
#if 0 /*JP*/
                Sprintf(buf, "You feel interested in %s financial situation.",
                        s_suffix(x_monnam(u.usteed,
                                          u.usteed->mtame ? ARTICLE_YOUR
                                                          : ARTICLE_THE,
                                          (char *) 0,
                                          SUPPRESS_SADDLE, FALSE)));
#else
                Sprintf(buf, "���Ȃ���%s�̌o�Ϗ󋵂ɋ������o�Ă����D",
                        x_monnam(u.usteed,
                                          u.usteed->mtame ? ARTICLE_YOUR
                                                          : ARTICLE_THE,
                                          (char *) 0,
                                          SUPPRESS_SADDLE, FALSE));
#endif
            else
/*JP
                Strcpy(buf, "You feel materially poor.");
*/
                Strcpy(buf, "���Ȃ��͂Ђ��������������D");

            strange_feeling(sobj, buf);
        }
        return 1;
    }
    /* only under me - no separate display required */
    if (stale)
        docrt();
/*JP
    You("notice some gold between your %s.", makeplural(body_part(FOOT)));
*/
    You("%s�̊Ԃɋ��݂������Ă��邱�ƂɋC�������D", body_part(FOOT));
    return 0;

outgoldmap:
    cls();

    (void) unconstrain_map();
    /* Discover gold locations. */
    for (obj = fobj; obj; obj = obj->nobj) {
        if (sobj->blessed && (temp = o_material(obj, GOLD)) != 0) {
            if (temp != obj) {
                temp->ox = obj->ox;
                temp->oy = obj->oy;
            }
            map_object(temp, 1);
        } else if ((temp = o_in(obj, COIN_CLASS)) != 0) {
            if (temp != obj) {
                temp->ox = obj->ox;
                temp->oy = obj->oy;
            }
            map_object(temp, 1);
        }
        if (temp && temp->ox == u.ux && temp->oy == u.uy)
            ugold = TRUE;
    }
    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp))
            continue; /* probably overkill here */
        temp = 0;
        if (findgold(mtmp->minvent) || monsndx(mtmp->data) == PM_GOLD_GOLEM) {
            gold = zeroobj; /* ensure oextra is cleared too */
            gold.otyp = GOLD_PIECE;
            gold.quan = (long) rnd(10); /* usually more than 1 */
            gold.ox = mtmp->mx;
            gold.oy = mtmp->my;
            map_object(&gold, 1);
            temp = &gold;
        } else {
            for (obj = mtmp->minvent; obj; obj = obj->nobj)
                if (sobj->blessed && (temp = o_material(obj, GOLD)) != 0) {
                    temp->ox = mtmp->mx;
                    temp->oy = mtmp->my;
                    map_object(temp, 1);
                    break;
                } else if ((temp = o_in(obj, COIN_CLASS)) != 0) {
                    temp->ox = mtmp->mx;
                    temp->oy = mtmp->my;
                    map_object(temp, 1);
                    break;
                }
        }
        if (temp && temp->ox == u.ux && temp->oy == u.uy)
            ugold = TRUE;
    }
    if (!ugold) {
        newsym(u.ux, u.uy);
        ter_typ |= TER_MON; /* so autodescribe will recognize hero */
    }
/*JP
    You_feel("very greedy, and sense gold!");
*/
    You("�ǂ�~�ɂȂ����悤�ȋC�������C�����ċ��݂̈ʒu�����m�����I");
    exercise(A_WIS, TRUE);

/*JP
    browse_map(ter_typ, "gold");
*/
    browse_map(ter_typ, "��");

    reconstrain_map();
    docrt();
    if (Underwater)
        under_water(2);
    if (u.uburied)
        under_ground(2);
    return 0;
}

/* returns 1 if nothing was detected   */
/* returns 0 if something was detected */
int
food_detect(sobj)
register struct obj *sobj;
{
    register struct obj *obj;
    register struct monst *mtmp;
    register int ct = 0, ctu = 0;
    boolean confused = (Confusion || (sobj && sobj->cursed)), stale;
    char oclass = confused ? POTION_CLASS : FOOD_CLASS;
/*JP
    const char *what = confused ? something : "food";
*/
    const char *what = confused ? "�n���w��" : "�H�ו�";

    stale = clear_stale_map(oclass, 0);
    if (u.usteed) /* some situations leave steed with stale coordinates */
        u.usteed->mx = u.ux, u.usteed->my = u.uy;

    for (obj = fobj; obj; obj = obj->nobj)
        if (o_in(obj, oclass)) {
            if (obj->ox == u.ux && obj->oy == u.uy)
                ctu++;
            else
                ct++;
        }
    for (mtmp = fmon; mtmp && (!ct || !ctu); mtmp = mtmp->nmon) {
        /* no DEADMONSTER(mtmp) check needed -- dmons never have inventory */
        for (obj = mtmp->minvent; obj; obj = obj->nobj)
            if (o_in(obj, oclass)) {
                if (mtmp->mx == u.ux && mtmp->my == u.uy)
                    ctu++; /* steed or an engulfer with inventory */
                else
                    ct++;
                break;
            }
    }

    if (!ct && !ctu) {
        known = stale && !confused;
        if (stale) {
            docrt();
/*JP
            You("sense a lack of %s nearby.", what);
*/
            You("%s�������Ă���̂ɋC�������D",what);
            if (sobj && sobj->blessed) {
                if (!u.uedibility)
/*JP
                    Your("%s starts to tingle.", body_part(NOSE));
*/
                    Your("%s���҂����Ɠ������D", body_part(NOSE));
                u.uedibility = 1;
            }
        } else if (sobj) {
            char buf[BUFSZ];

#if 0 /*JP:T*/
            Sprintf(buf, "Your %s twitches%s.", body_part(NOSE),
                    (sobj->blessed && !u.uedibility)
                        ? " then starts to tingle"
                        : "");
#else
            Sprintf(buf, "���Ȃ���%s���Ђ��Ђ��Ɠ���%s�D", body_part(NOSE),
                    (sobj->blessed && !u.uedibility)
                        ? "�āC����������������"
                        : "��");
#endif
            if (sobj->blessed && !u.uedibility) {
                boolean savebeginner = flags.beginner;

                flags.beginner = FALSE; /* prevent non-delivery of message */
                strange_feeling(sobj, buf);
                flags.beginner = savebeginner;
                u.uedibility = 1;
            } else
                strange_feeling(sobj, buf);
        }
        return !stale;
    } else if (!ct) {
        known = TRUE;
/*JP
        You("%s %s nearby.", sobj ? "smell" : "sense", what);
*/
        You("�߂���%s%s�D", what, sobj ? "�̂ɂ�����������" : "�����m����");
        if (sobj && sobj->blessed) {
            if (!u.uedibility)
/*JP
                pline("Your %s starts to tingle.", body_part(NOSE));
*/
                pline("���Ȃ���%s�������������������D", body_part(NOSE));
            u.uedibility = 1;
        }
    } else {
        struct obj *temp;
        int ter_typ = TER_DETECT | TER_OBJ;

        known = TRUE;
        cls();
        (void) unconstrain_map();
        for (obj = fobj; obj; obj = obj->nobj)
            if ((temp = o_in(obj, oclass)) != 0) {
                if (temp != obj) {
                    temp->ox = obj->ox;
                    temp->oy = obj->oy;
                }
                map_object(temp, 1);
            }
        for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
            /* no DEADMONSTER() check needed -- dmons never have inventory */
            for (obj = mtmp->minvent; obj; obj = obj->nobj)
                if ((temp = o_in(obj, oclass)) != 0) {
                    temp->ox = mtmp->mx;
                    temp->oy = mtmp->my;
                    map_object(temp, 1);
                    break; /* skip rest of this monster's inventory */
                }
        if (!ctu) {
            newsym(u.ux, u.uy);
            ter_typ |= TER_MON; /* for autodescribe of self */
        }
        if (sobj) {
            if (sobj->blessed) {
#if 0 /*JP*/
                Your("%s %s to tingle and you smell %s.", body_part(NOSE),
                     u.uedibility ? "continues" : "starts", what);
#else
                Your("%s�͂���������%s�C%s�̓�����k���Ƃ����D", body_part(NOSE),
                     u.uedibility ? "����" : "�n��", what);
#endif
                u.uedibility = 1;
            } else
/*JP
                Your("%s tingles and you smell %s.", body_part(NOSE), what);
*/
                Your("%s�͂����������C%s�̓�����k���Ƃ����D", body_part(NOSE), what);
        } else
/*JP
            You("sense %s.", what);
*/
            You("%s�����m�����D", what);
        exercise(A_WIS, TRUE);

/*JP
        browse_map(ter_typ, "food");
*/
        browse_map(ter_typ, "�H��");

        reconstrain_map();
        docrt();
        if (Underwater)
            under_water(2);
        if (u.uburied)
            under_ground(2);
    }
    return 0;
}

/*
 * Used for scrolls, potions, spells, and crystal balls.  Returns:
 *
 *      1 - nothing was detected
 *      0 - something was detected
 */
int
object_detect(detector, class)
struct obj *detector; /* object doing the detecting */
int class;            /* an object class, 0 for all */
{
    register int x, y;
    char stuff[BUFSZ];
    int is_cursed = (detector && detector->cursed);
    int do_dknown = (detector && (detector->oclass == POTION_CLASS
                                  || detector->oclass == SPBOOK_CLASS)
                     && detector->blessed);
    int ct = 0, ctu = 0;
    register struct obj *obj, *otmp = (struct obj *) 0;
    register struct monst *mtmp;
    int sym, boulder = 0, ter_typ = TER_DETECT | TER_OBJ;

    if (class < 0 || class >= MAXOCLASSES) {
        impossible("object_detect:  illegal class %d", class);
        class = 0;
    }

    /* Special boulder symbol check - does the class symbol happen
     * to match iflags.bouldersym which is a user-defined?
     * If so, that means we aren't sure what they really wanted to
     * detect. Rather than trump anything, show both possibilities.
     * We can exclude checking the buried obj chain for boulders below.
     */
    sym = class ? def_oc_syms[class].sym : 0;
    if (sym && iflags.bouldersym && sym == iflags.bouldersym)
        boulder = ROCK_CLASS;

    if (Hallucination || (Confusion && class == SCROLL_CLASS))
        Strcpy(stuff, something);
    else
/*JP
        Strcpy(stuff, class ? def_oc_syms[class].name : "objects");
*/
        Strcpy(stuff, class ? def_oc_syms[class].name : "����");
    if (boulder && class != ROCK_CLASS)
/*JP
        Strcat(stuff, " and/or large stones");
*/
        Strcat(stuff, "�Ƌ���");

    if (do_dknown)
        for (obj = invent; obj; obj = obj->nobj)
            do_dknown_of(obj);

    for (obj = fobj; obj; obj = obj->nobj) {
        if ((!class && !boulder) || o_in(obj, class) || o_in(obj, boulder)) {
            if (obj->ox == u.ux && obj->oy == u.uy)
                ctu++;
            else
                ct++;
        }
        if (do_dknown)
            do_dknown_of(obj);
    }

    for (obj = level.buriedobjlist; obj; obj = obj->nobj) {
        if (!class || o_in(obj, class)) {
            if (obj->ox == u.ux && obj->oy == u.uy)
                ctu++;
            else
                ct++;
        }
        if (do_dknown)
            do_dknown_of(obj);
    }

    if (u.usteed)
        u.usteed->mx = u.ux, u.usteed->my = u.uy;

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp))
            continue;
        for (obj = mtmp->minvent; obj; obj = obj->nobj) {
            if ((!class && !boulder) || o_in(obj, class)
                || o_in(obj, boulder))
                ct++;
            if (do_dknown)
                do_dknown_of(obj);
        }
        if ((is_cursed && mtmp->m_ap_type == M_AP_OBJECT
             && (!class || class == objects[mtmp->mappearance].oc_class))
            || (findgold(mtmp->minvent) && (!class || class == COIN_CLASS))) {
            ct++;
            break;
        }
    }

    if (!clear_stale_map(!class ? ALL_CLASSES : class, 0) && !ct) {
        if (!ctu) {
            if (detector)
/*JP
                strange_feeling(detector, "You feel a lack of something.");
*/
                strange_feeling(detector, "���Ȃ��͉��������R���Ă���悤�ȋC�������D");
            return 1;
        }

/*JP
        You("sense %s nearby.", stuff);
*/
        You("�߂���%s�����m�����D", stuff);
        return 0;
    }

    cls();

    (void) unconstrain_map();
    /*
     *  Map all buried objects first.
     */
    for (obj = level.buriedobjlist; obj; obj = obj->nobj)
        if (!class || (otmp = o_in(obj, class)) != 0) {
            if (class) {
                if (otmp != obj) {
                    otmp->ox = obj->ox;
                    otmp->oy = obj->oy;
                }
                map_object(otmp, 1);
            } else
                map_object(obj, 1);
        }
    /*
     * If we are mapping all objects, map only the top object of a pile or
     * the first object in a monster's inventory.  Otherwise, go looking
     * for a matching object class and display the first one encountered
     * at each location.
     *
     * Objects on the floor override buried objects.
     */
    for (x = 1; x < COLNO; x++)
        for (y = 0; y < ROWNO; y++)
            for (obj = level.objects[x][y]; obj; obj = obj->nexthere)
                if ((!class && !boulder) || (otmp = o_in(obj, class)) != 0
                    || (otmp = o_in(obj, boulder)) != 0) {
                    if (class || boulder) {
                        if (otmp != obj) {
                            otmp->ox = obj->ox;
                            otmp->oy = obj->oy;
                        }
                        map_object(otmp, 1);
                    } else
                        map_object(obj, 1);
                    break;
                }

    /* Objects in the monster's inventory override floor objects. */
    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp))
            continue;
        for (obj = mtmp->minvent; obj; obj = obj->nobj)
            if ((!class && !boulder) || (otmp = o_in(obj, class)) != 0
                || (otmp = o_in(obj, boulder)) != 0) {
                if (!class && !boulder)
                    otmp = obj;
                otmp->ox = mtmp->mx; /* at monster location */
                otmp->oy = mtmp->my;
                map_object(otmp, 1);
                break;
            }
        /* Allow a mimic to override the detected objects it is carrying. */
        if (is_cursed && mtmp->m_ap_type == M_AP_OBJECT
            && (!class || class == objects[mtmp->mappearance].oc_class)) {
            struct obj temp;

            temp = zeroobj;
            temp.otyp = mtmp->mappearance; /* needed for obj_to_glyph() */
            temp.quan = 1L;
            temp.ox = mtmp->mx;
            temp.oy = mtmp->my;
            temp.corpsenm = PM_TENGU; /* if mimicing a corpse */
            map_object(&temp, 1);
        } else if (findgold(mtmp->minvent)
                   && (!class || class == COIN_CLASS)) {
            struct obj gold;

            gold = zeroobj; /* ensure oextra is cleared too */
            gold.otyp = GOLD_PIECE;
            gold.quan = (long) rnd(10); /* usually more than 1 */
            gold.ox = mtmp->mx;
            gold.oy = mtmp->my;
            map_object(&gold, 1);
        }
    }
    if (!glyph_is_object(glyph_at(u.ux, u.uy))) {
        newsym(u.ux, u.uy);
        ter_typ |= TER_MON;
    }
/*JP
    You("detect the %s of %s.", ct ? "presence" : "absence", stuff);
*/
    You("%s%s�D", stuff, ct ? "�𔭌�����" : "�͉����Ȃ����Ƃ��킩����" );

    if (!ct)
        display_nhwindow(WIN_MAP, TRUE);
    else
/*JP
        browse_map(ter_typ, "object");
*/
        browse_map(ter_typ, "����");

    reconstrain_map();
    docrt(); /* this will correctly reset vision */
    if (Underwater)
        under_water(2);
    if (u.uburied)
        under_ground(2);
    return 0;
}

/*
 * Used by: crystal balls, potions, fountains
 *
 * Returns 1 if nothing was detected.
 * Returns 0 if something was detected.
 */
int
monster_detect(otmp, mclass)
register struct obj *otmp; /* detecting object (if any) */
int mclass;                /* monster class, 0 for all */
{
    register struct monst *mtmp;
    int mcnt = 0;

    /* Note: This used to just check fmon for a non-zero value
     * but in versions since 3.3.0 fmon can test TRUE due to the
     * presence of dmons, so we have to find at least one
     * with positive hit-points to know for sure.
     */
    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
        if (!DEADMONSTER(mtmp)) {
            mcnt++;
            break;
        }

    if (!mcnt) {
        if (otmp)
            strange_feeling(otmp, Hallucination
/*JP
                                      ? "You get the heebie jeebies."
*/
                                      ? "���Ȃ��͋����̉ĂŃL���`���[�����D"
/*JP
                                      : "You feel threatened.");
*/
                                      : "���Ȃ��͋��|�ł������Ƃ����D");
        return 1;
    } else {
        boolean unconstrained, woken = FALSE;
        unsigned swallowed = u.uswallow; /* before unconstrain_map() */

        cls();
        unconstrained = unconstrain_map();
        for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
            if (DEADMONSTER(mtmp))
                continue;
            if (!mclass || mtmp->data->mlet == mclass
                || (mtmp->data == &mons[PM_LONG_WORM]
                    && mclass == S_WORM_TAIL))
                map_monst(mtmp, TRUE);

            if (otmp && otmp->cursed
                && (mtmp->msleeping || !mtmp->mcanmove)) {
                mtmp->msleeping = mtmp->mfrozen = 0;
                mtmp->mcanmove = 1;
                woken = TRUE;
            }
        }
        if (!swallowed)
            display_self();
/*JP
        You("sense the presence of monsters.");
*/
        You("�����̑��݂�k�������D");
        if (woken)
/*JP
            pline("Monsters sense the presence of you.");
*/
            pline("�����͂��Ȃ��̑��݂�k�������D");

        if ((otmp && otmp->blessed) && !unconstrained) {
            /* persistent detection--just show updated map */
            display_nhwindow(WIN_MAP, TRUE);
        } else {
            /* one-shot detection--allow player to move cursor around and
               get autodescribe feedback */
            EDetect_monsters |= I_SPECIAL;
/*JP
            browse_map(TER_DETECT | TER_MON, "monster of interest");
*/
            browse_map(TER_DETECT | TER_MON, "�֐S�̂������");
            EDetect_monsters &= ~I_SPECIAL;
        }

        reconstrain_map();
        docrt(); /* redraw the screen to remove unseen monsters from map */
        if (Underwater)
            under_water(2);
        if (u.uburied)
            under_ground(2);
    }
    return 0;
}

STATIC_OVL void
sense_trap(trap, x, y, src_cursed)
struct trap *trap;
xchar x, y;
int src_cursed;
{
    if (Hallucination || src_cursed) {
        struct obj obj; /* fake object */

        obj = zeroobj;
        if (trap) {
            obj.ox = trap->tx;
            obj.oy = trap->ty;
        } else {
            obj.ox = x;
            obj.oy = y;
        }
        obj.otyp = !Hallucination ? GOLD_PIECE : random_object();
        obj.quan = (long) ((obj.otyp == GOLD_PIECE) ? rnd(10)
                           : objects[obj.otyp].oc_merge ? rnd(2) : 1);
        obj.corpsenm = random_monster(); /* if otyp == CORPSE */
        map_object(&obj, 1);
    } else if (trap) {
        map_trap(trap, 1);
        trap->tseen = 1;
    } else { /* trapped door or trapped chest */
        struct trap temp_trap; /* fake trap */

        (void) memset((genericptr_t) &temp_trap, 0, sizeof temp_trap);
        temp_trap.tx = x;
        temp_trap.ty = y;
        temp_trap.ttyp = BEAR_TRAP; /* some kind of trap */
        map_trap(&temp_trap, 1);
    }
}

#define OTRAP_NONE 0  /* nothing found */
#define OTRAP_HERE 1  /* found at hero's location */
#define OTRAP_THERE 2 /* found at any other location */

/* check a list of objects for chest traps; return 1 if found at <ux,uy>,
   2 if found at some other spot, 3 if both, 0 otherwise; optionally
   update the map to show where such traps were found */
STATIC_OVL int
detect_obj_traps(objlist, show_them, how)
struct obj *objlist;
boolean show_them;
int how; /* 1 for misleading map feedback */
{
    struct obj *otmp;
    xchar x, y;
    int result = OTRAP_NONE;

    /*
     * TODO?  Display locations of unarmed land mine and beartrap objects.
     * If so, should they be displayed as objects or as traps?
     */

    for (otmp = objlist; otmp; otmp = otmp->nobj) {
        if (Is_box(otmp) && otmp->otrapped
            && get_obj_location(otmp, &x, &y, BURIED_TOO | CONTAINED_TOO)) {
            result |= (x == u.ux && y == u.uy) ? OTRAP_HERE : OTRAP_THERE;
            if (show_them)
                sense_trap((struct trap *) 0, x, y, how);
        }
        if (Has_contents(otmp))
            result |= detect_obj_traps(otmp->cobj, show_them, how);
    }
    return result;
}

/* the detections are pulled out so they can
 * also be used in the crystal ball routine
 * returns 1 if nothing was detected
 * returns 0 if something was detected
 */
int
trap_detect(sobj)
struct obj *sobj; /* null if crystal ball, *scroll if gold detection scroll */
{
    register struct trap *ttmp;
    struct monst *mon;
    int door, glyph, tr, ter_typ = TER_DETECT | TER_TRP;
    int cursed_src = sobj && sobj->cursed;
    boolean found = FALSE;
    coord cc;

    if (u.usteed)
        u.usteed->mx = u.ux, u.usteed->my = u.uy;

    /* floor/ceiling traps */
    for (ttmp = ftrap; ttmp; ttmp = ttmp->ntrap) {
        if (ttmp->tx != u.ux || ttmp->ty != u.uy)
            goto outtrapmap;
        else
            found = TRUE;
    }
    /* chest traps (might be buried or carried) */
    if ((tr = detect_obj_traps(fobj, FALSE, 0)) != OTRAP_NONE) {
        if (tr & OTRAP_THERE)
            goto outtrapmap;
        else
            found = TRUE;
    }
    if ((tr = detect_obj_traps(level.buriedobjlist, FALSE, 0)) != OTRAP_NONE) {
        if (tr & OTRAP_THERE)
            goto outtrapmap;
        else
            found = TRUE;
    }
    for (mon = fmon; mon; mon = mon->nmon) {
        if (DEADMONSTER(mon))
            continue;
        if ((tr = detect_obj_traps(mon->minvent, FALSE, 0)) != OTRAP_NONE) {
            if (tr & OTRAP_THERE)
                goto outtrapmap;
            else
                found = TRUE;
        }
    }
    if (detect_obj_traps(invent, FALSE, 0) != OTRAP_NONE)
        found = TRUE;
    /* door traps */
    for (door = 0; door < doorindex; door++) {
        cc = doors[door];
        if (levl[cc.x][cc.y].doormask & D_TRAPPED) {
            if (cc.x != u.ux || cc.y != u.uy)
                goto outtrapmap;
            else
                found = TRUE;
        }
    }
    if (!found) {
        char buf[BUFSZ];

/*JP
        Sprintf(buf, "Your %s stop itching.", makeplural(body_part(TOE)));
*/
        Sprintf(buf, "���Ȃ���%s�̂ނ��ނ��͂����܂����D", makeplural(body_part(TOE)));
        strange_feeling(sobj, buf);
        return 1;
    }
    /* traps exist, but only under me - no separate display required */
/*JP
    Your("%s itch.", makeplural(body_part(TOE)));
*/
    Your("%s�͂ނ��ނ������D", makeplural(body_part(TOE)));
    return 0;

outtrapmap:
    cls();

    (void) unconstrain_map();
    /* show chest traps first, so that subsequent floor trap display
       will override if both types are present at the same location */
    (void) detect_obj_traps(fobj, TRUE, cursed_src);
    (void) detect_obj_traps(level.buriedobjlist, TRUE, cursed_src);
    for (mon = fmon; mon; mon = mon->nmon) {
        if (DEADMONSTER(mon))
            continue;
        (void) detect_obj_traps(mon->minvent, TRUE, cursed_src);
    }
    (void) detect_obj_traps(invent, TRUE, cursed_src);

    for (ttmp = ftrap; ttmp; ttmp = ttmp->ntrap)
        sense_trap(ttmp, 0, 0, cursed_src);

    for (door = 0; door < doorindex; door++) {
        cc = doors[door];
        if (levl[cc.x][cc.y].doormask & D_TRAPPED)
            sense_trap((struct trap *) 0, cc.x, cc.y, cursed_src);
    }

    /* redisplay hero unless sense_trap() revealed something at <ux,uy> */
    glyph = glyph_at(u.ux, u.uy);
    if (!(glyph_is_trap(glyph) || glyph_is_object(glyph))) {
        newsym(u.ux, u.uy);
        ter_typ |= TER_MON; /* for autodescribe at <u.ux,u.uy> */
    }
/*JP
    You_feel("%s.", cursed_src ? "very greedy" : "entrapped");
*/
    You("%s�C���ɂȂ����D", cursed_src ? "�ƂĂ��ǂ�~��" : "���܂���Ă���悤��");

/*JP
    browse_map(ter_typ, "trap of interest");
*/
    browse_map(ter_typ, "�֐S�̂����");

    reconstrain_map();
    docrt(); /* redraw the screen to remove unseen traps from the map */
    if (Underwater)
        under_water(2);
    if (u.uburied)
        under_ground(2);
    return 0;
}

const char *
level_distance(where)
d_level *where;
{
    register schar ll = depth(&u.uz) - depth(where);
    register boolean indun = (u.uz.dnum == where->dnum);

    if (ll < 0) {
        if (ll < (-8 - rn2(3)))
            if (!indun)
/*JP
                return "far away";
*/
              return "�͂邩�ޕ���";
            else
/*JP
                return "far below";
*/
        return "�͂邩������";
        else if (ll < -1)
            if (!indun)
/*JP
                return "away below you";
*/
              return "�����Ɖ�����";
            else
/*JP
                return "below you";
*/
              return "������";
        else if (!indun)
/*JP
            return "in the distance";
*/
          return "������";
        else
/*JP
            return "just below";
*/
          return "�^����";
    } else if (ll > 0) {
        if (ll > (8 + rn2(3)))
            if (!indun)
/*JP
                return "far away";
*/
              return "�͂邩�ޕ���";
            else
/*JP
                return "far above";
*/
              return "�͂邩�����";
        else if (ll > 1)
            if (!indun)
/*JP
                return "away above you";
*/
              return "�����Ə����";
            else
/*JP
                return "above you";
*/
              return "�����";
        else if (!indun)
/*JP
            return "in the distance";
*/
          return "������";
        else
/*JP
            return "just above";
*/
          return "�^���";
    } else if (!indun)
/*JP
        return "in the distance";
*/
      return "������";
    else
/*JP
        return "near you";
*/
      return "�߂���";
}

static const struct {
    const char *what;
    d_level *where;
} level_detects[] = {
/*JP
    { "Delphi", &oracle_level },
*/
    { "�f���t�@�C", &oracle_level },
/*JP
    { "Medusa's lair", &medusa_level },
*/
    { "���f���[�T�̏Z�݂�", &medusa_level },
/*JP
    { "a castle", &stronghold_level },
*/
    { "��", &stronghold_level },
/*JP
    { "the Wizard of Yendor's tower", &wiz1_level },
*/
    { "�C�F���_�[�̖��@�g���̓�", &wiz1_level },
};

void
use_crystal_ball(optr)
struct obj **optr;
{
    char ch;
    int oops;
    struct obj *obj = *optr;

    if (Blind) {
/*JP
        pline("Too bad you can't see %s.", the(xname(obj)));
*/
        pline("�Ȃ�Ă��Ƃ��D%s�����邱�Ƃ��ł��Ȃ��D", the(xname(obj)));
        return;
    }
    oops = (rnd(20) > ACURR(A_INT) || obj->cursed);
    if (oops && (obj->spe > 0)) {
        switch (rnd(obj->oartifact ? 4 : 5)) {
        case 1:
/*JP
            pline("%s too much to comprehend!", Tobjnam(obj, "are"));
*/
            pline("%s��`���������̂��Ƃ��������ς�킩��Ȃ������I", xname(obj));
            break;
        case 2:
/*JP
            pline("%s you!", Tobjnam(obj, "confuse"));
*/
            pline("%s��`���Ă�Ƃӂ���Ă����I", xname(obj));
            make_confused((HConfusion & TIMEOUT) + (long) rnd(100), FALSE);
            break;
        case 3:
            if (!resists_blnd(&youmonst)) {
/*JP
                pline("%s your vision!", Tobjnam(obj, "damage"));
*/
                pline("%s��`���Ă���Ǝ��o�����������Ȃ��Ă����I", xname(obj));
                make_blinded((Blinded & TIMEOUT) + (long) rnd(100), FALSE);
                if (!Blind)
                    Your1(vision_clears);
            } else {
/*JP
                pline("%s your vision.", Tobjnam(obj, "assault"));
*/
                pline("%s�����Ȃ��̎��E�ɔ����Ă����D", xname(obj));
/*JP
                You("are unaffected!");
*/
                pline("�������C���Ȃ��͉e�����󂯂Ȃ������I");
            }
            break;
        case 4:
/*JP
            pline("%s your mind!", Tobjnam(obj, "zap"));
*/
            pline("%s��`���Ă���ƌ܊������������Ȃ��Ă����I", xname(obj));
            (void) make_hallucinated(
                (HHallucination & TIMEOUT) + (long) rnd(100), FALSE, 0L);
            break;
        case 5:
/*JP
            pline("%s!", Tobjnam(obj, "explode"));
*/
            pline("%s�͔��������I", xname(obj));
            useup(obj);
            *optr = obj = 0; /* it's gone */
            /* physical damage cause by the shards and force */
/*JP
            losehp(Maybe_Half_Phys(rnd(30)), "exploding crystal ball",
*/
            losehp(Maybe_Half_Phys(rnd(30)), "�����ʂ̔�����",
                   KILLED_BY_AN);
            break;
        }
        if (obj)
            consume_obj_charge(obj, TRUE);
        return;
    }

    if (Hallucination) {
        if (!obj->spe) {
/*JP
            pline("All you see is funky %s haze.", hcolor((char *) 0));
*/
            pline("�����I�t�@���L�[�����L�[��%s���₪������D", hcolor((char *)0));
        } else {
            switch (rnd(6)) {
            case 1:
/*JP
                You("grok some groovy globs of incandescent lava.");
*/
                You("����̌������������ˉ��傪�ǂ̉e�ɉB��Ă���̂��������D");
                break;
            case 2:
#if 0 /*JP:T*/
                pline("Whoa!  Psychedelic colors, %s!",
                      poly_gender() == 1 ? "babe" : "dude");
#else
                pline("���[�I�I�������Ă邩���H%s�I",
                      poly_gender() == 1 ? "�x�C�r�[" : "���[");
#endif
                break;
            case 3:
/*JP
                pline_The("crystal pulses with sinister %s light!",
*/
                pline("�����͕s�g��%s�p���X�𔭂����I", 
                          hcolor((char *) 0));
                break;
            case 4:
/*JP
                You_see("goldfish swimming above fluorescent rocks.");
*/
                You("�u����̏���������j���ł���̂������D");
                break;
            case 5:
#if 0 /*JP*/
                You_see(
                    "tiny snowflakes spinning around a miniature farmhouse.");
#else
                    You("��������Ђ��~�j�`���A�̔_�Ƃ̉Ƃ̂܂��𕑂��Ă�̂������D");
#endif
                break;
            default:
/*JP
                pline("Oh wow... like a kaleidoscope!");
*/
                pline("���[�I�D���؋��̂悤���I");
                break;
            }
            consume_obj_charge(obj, TRUE);
        }
        return;
    }

    /* read a single character */
    if (flags.verbose)
/*JP
        You("may look for an object or monster symbol.");
*/
        You("���̂�����̋L����T����D");
/*JP
    ch = yn_function("What do you look for?", (char *) 0, '\0');
*/
    ch = yn_function("����T���܂����H", (char *)0, '\0');
    /* Don't filter out ' ' here; it has a use */
    if ((ch != def_monsyms[S_GHOST].sym) && index(quitchars, ch)) {
        if (flags.verbose)
            pline1(Never_mind);
        return;
    }
/*JP
    You("peer into %s...", the(xname(obj)));
*/
    You("%s��`�����񂾁D�D�D", the(xname(obj)));
    nomul(-rnd(10));
/*JP
    multi_reason = "gazing into a crystal ball";
*/
    multi_reason = "��������`������ł��鎞��";
    nomovemsg = "";
    if (obj->spe <= 0) {
/*JP
        pline_The("vision is unclear.");
*/
        pline("�f���͕s�N���������D");
    } else {
        int class, i;
        int ret = 0;

        makeknown(CRYSTAL_BALL);
        consume_obj_charge(obj, TRUE);

        /* special case: accept ']' as synonym for mimic
         * we have to do this before the def_char_to_objclass check
         */
        if (ch == DEF_MIMIC_DEF)
            ch = DEF_MIMIC;

        if ((class = def_char_to_objclass(ch)) != MAXOCLASSES)
            ret = object_detect((struct obj *) 0, class);
        else if ((class = def_char_to_monclass(ch)) != MAXMCLASSES)
            ret = monster_detect((struct obj *) 0, class);
        else if (iflags.bouldersym && (ch == iflags.bouldersym))
            ret = object_detect((struct obj *) 0, ROCK_CLASS);
        else
            switch (ch) {
            case '^':
                ret = trap_detect((struct obj *) 0);
                break;
            default:
                i = rn2(SIZE(level_detects));
#if 0 /*JP:T*/
                You_see("%s, %s.", level_detects[i].what,
                        level_distance(level_detects[i].where));
#else
                You_see("%s��%s�����D", level_detects[i].what,
                        level_distance(level_detects[i].where));
#endif
                ret = 0;
                break;
            }

        if (ret) {
            if (!rn2(100)) /* make them nervous */
/*JP
                You_see("the Wizard of Yendor gazing out at you.");
*/
                You("�C�F���_�[�̖��@�g�������Ȃ����ɂ��ł���̂������D");
            else
/*JP
                pline_The("vision is unclear.");
*/
                pline("�f���͕s�N���ɂȂ����D");
        }
    }
    return;
}

STATIC_OVL void
show_map_spot(x, y)
register int x, y;
{
    struct rm *lev;
    struct trap *t;
    int oldglyph;

    if (Confusion && rn2(7))
        return;
    lev = &levl[x][y];

    lev->seenv = SVALL;

    /* Secret corridors are found, but not secret doors. */
    if (lev->typ == SCORR) {
        lev->typ = CORR;
        unblock_point(x, y);
    }

    /*
     * Force the real background, then if it's not furniture and there's
     * a known trap there, display the trap, else if there was an object
     * shown there, redisplay the object.  So during mapping, furniture
     * takes precedence over traps, which take precedence over objects,
     * opposite to how normal vision behaves.
     */
    oldglyph = glyph_at(x, y);
    if (level.flags.hero_memory) {
        magic_map_background(x, y, 0);
        newsym(x, y); /* show it, if not blocked */
    } else {
        magic_map_background(x, y, 1); /* display it */
    }
    if (!IS_FURNITURE(lev->typ)) {
        if ((t = t_at(x, y)) != 0 && t->tseen) {
            map_trap(t, 1);
        } else if (glyph_is_trap(oldglyph) || glyph_is_object(oldglyph)) {
            show_glyph(x, y, oldglyph);
            if (level.flags.hero_memory)
                lev->glyph = oldglyph;
        }
    }
}

void
do_mapping()
{
    register int zx, zy;
    boolean unconstrained;

    unconstrained = unconstrain_map();
    for (zx = 1; zx < COLNO; zx++)
        for (zy = 0; zy < ROWNO; zy++)
            show_map_spot(zx, zy);

    if (!level.flags.hero_memory || unconstrained) {
        flush_screen(1);                 /* flush temp screen */
        /* browse_map() instead of display_nhwindow(WIN_MAP, TRUE) */
#if 0 /*JP*/
        browse_map(TER_DETECT | TER_MAP | TER_TRP | TER_OBJ,
                   "anything of interest");
#else
        browse_map(TER_DETECT | TER_MAP | TER_TRP | TER_OBJ,
                   "�֐S�̂������");
#endif
        docrt();
    }
    reconstrain_map();
    exercise(A_WIS, TRUE);
}

/* clairvoyance */
void
do_vicinity_map(sobj)
struct obj *sobj; /* scroll--actually fake spellbook--object */
{
    register int zx, zy;
    struct monst *mtmp;
    boolean unconstrained, refresh = FALSE, mdetected = FALSE,
            extended = (sobj && sobj->blessed);
    int lo_y = ((u.uy - 5 < 0) ? 0 : u.uy - 5),
        hi_y = ((u.uy + 6 >= ROWNO) ? ROWNO - 1 : u.uy + 6),
        lo_x = ((u.ux - 9 < 1) ? 1 : u.ux - 9), /* avoid column 0 */
        hi_x = ((u.ux + 10 >= COLNO) ? COLNO - 1 : u.ux + 10),
        ter_typ = TER_DETECT | TER_MAP | TER_TRP | TER_OBJ;

    unconstrained = unconstrain_map();
    for (zx = lo_x; zx <= hi_x; zx++)
        for (zy = lo_y; zy <= hi_y; zy++) {
            show_map_spot(zx, zy);

            if (extended && (mtmp = m_at(zx, zy)) != 0
                && mtmp->mx == zx && mtmp->my == zy) { /* skip worm tails */
                int oldglyph = glyph_at(zx, zy);

                map_monst(mtmp, FALSE);
                if (glyph_at(zx, zy) != oldglyph)
                    mdetected = TRUE;
            }
        }

    if (!level.flags.hero_memory || unconstrained || mdetected) {
        flush_screen(1);                 /* flush temp screen */
        /* the getpos() prompt from browse_map() is only shown when
           flags.verbose is set, but make this unconditional so that
           not-verbose users become aware of the prompting situation */
/*JP
        You("sense your surroundings.");
*/
        You("�܂��̂��̂����m�����D");
        if (extended || glyph_is_monster(glyph_at(u.ux, u.uy)))
            ter_typ |= TER_MON;
        if (extended)
            EDetect_monsters |= I_SPECIAL;
/*JP
        browse_map(ter_typ, "anything of interest");
*/
        browse_map(ter_typ, "�֐S�̂������");
        EDetect_monsters &= ~I_SPECIAL;
        refresh = TRUE;
    }
    reconstrain_map();
    if (refresh)
        docrt();
}

/* convert a secret door into a normal door */
void
cvt_sdoor_to_door(lev)
struct rm *lev;
{
    int newmask = lev->doormask & ~WM_MASK;

    if (Is_rogue_level(&u.uz))
        /* rogue didn't have doors, only doorways */
        newmask = D_NODOOR;
    else
        /* newly exposed door is closed */
        if (!(newmask & D_LOCKED))
        newmask |= D_CLOSED;

    lev->typ = DOOR;
    lev->doormask = newmask;
}

STATIC_PTR void
findone(zx, zy, num)
int zx, zy;
genericptr_t num;
{
    register struct trap *ttmp;
    register struct monst *mtmp;

    if (levl[zx][zy].typ == SDOOR) {
        cvt_sdoor_to_door(&levl[zx][zy]); /* .typ = DOOR */
        magic_map_background(zx, zy, 0);
        newsym(zx, zy);
        (*(int *) num)++;
    } else if (levl[zx][zy].typ == SCORR) {
        levl[zx][zy].typ = CORR;
        unblock_point(zx, zy);
        magic_map_background(zx, zy, 0);
        newsym(zx, zy);
        (*(int *) num)++;
    } else if ((ttmp = t_at(zx, zy)) != 0) {
        if (!ttmp->tseen && ttmp->ttyp != STATUE_TRAP) {
            ttmp->tseen = 1;
            newsym(zx, zy);
            (*(int *) num)++;
        }
    } else if ((mtmp = m_at(zx, zy)) != 0) {
        if (mtmp->m_ap_type) {
            seemimic(mtmp);
            (*(int *) num)++;
        }
        if (mtmp->mundetected
            && (is_hider(mtmp->data) || mtmp->data->mlet == S_EEL)) {
            mtmp->mundetected = 0;
            newsym(zx, zy);
            (*(int *) num)++;
        }
        if (!canspotmon(mtmp) && !glyph_is_invisible(levl[zx][zy].glyph))
            map_invisible(zx, zy);
    } else if (unmap_invisible(zx, zy)) {
        (*(int *) num)++;
    }
}

STATIC_PTR void
openone(zx, zy, num)
int zx, zy;
genericptr_t num;
{
    register struct trap *ttmp;
    register struct obj *otmp;
    int *num_p = (int *) num;

    if (OBJ_AT(zx, zy)) {
        for (otmp = level.objects[zx][zy]; otmp; otmp = otmp->nexthere) {
            if (Is_box(otmp) && otmp->olocked) {
                otmp->olocked = 0;
                (*num_p)++;
            }
        }
        /* let it fall to the next cases. could be on trap. */
    }
    if (levl[zx][zy].typ == SDOOR
        || (levl[zx][zy].typ == DOOR
            && (levl[zx][zy].doormask & (D_CLOSED | D_LOCKED)))) {
        if (levl[zx][zy].typ == SDOOR)
            cvt_sdoor_to_door(&levl[zx][zy]); /* .typ = DOOR */
        if (levl[zx][zy].doormask & D_TRAPPED) {
            if (distu(zx, zy) < 3)
/*JP
                b_trapped("door", 0);
*/
                b_trapped("��", 0);
            else
#if 0 /*JP*/
                Norep("You %s an explosion!",
                      cansee(zx, zy) ? "see" : (!Deaf ? "hear"
                                                      : "feel the shock of"));
#else
                Norep("���Ȃ��͔���%s�I",
                      cansee(zx, zy) ? "������" : (!Deaf ? "���𕷂���"
                                                         : "�̏Ռ���������"));
#endif
            wake_nearto(zx, zy, 11 * 11);
            levl[zx][zy].doormask = D_NODOOR;
        } else
            levl[zx][zy].doormask = D_ISOPEN;
        unblock_point(zx, zy);
        newsym(zx, zy);
        (*num_p)++;
    } else if (levl[zx][zy].typ == SCORR) {
        levl[zx][zy].typ = CORR;
        unblock_point(zx, zy);
        newsym(zx, zy);
        (*num_p)++;
    } else if ((ttmp = t_at(zx, zy)) != 0) {
        struct monst *mon;
        boolean dummy; /* unneeded "you notice it arg" */

        if (!ttmp->tseen && ttmp->ttyp != STATUE_TRAP) {
            ttmp->tseen = 1;
            newsym(zx, zy);
            (*num_p)++;
        }
        mon = (zx == u.ux && zy == u.uy) ? &youmonst : m_at(zx, zy);
        if (openholdingtrap(mon, &dummy)
            || openfallingtrap(mon, TRUE, &dummy))
            (*num_p)++;
    } else if (find_drawbridge(&zx, &zy)) {
        /* make sure it isn't an open drawbridge */
        open_drawbridge(zx, zy);
        (*num_p)++;
    }
}

/* returns number of things found */
int
findit()
{
    int num = 0;

    if (u.uswallow)
        return 0;
    do_clear_area(u.ux, u.uy, BOLT_LIM, findone, (genericptr_t) &num);
    return num;
}

/* returns number of things found and opened */
int
openit()
{
    int num = 0;

    if (u.uswallow) {
        if (is_animal(u.ustuck->data)) {
            if (Blind)
/*JP
                pline("Its mouth opens!");
*/
                pline("���҂��̌����J�����I");
            else
/*JP
                pline("%s opens its mouth!", Monnam(u.ustuck));
*/
                pline("%s�͌����J�����I", Monnam(u.ustuck));
        }
        expels(u.ustuck, u.ustuck->data, TRUE);
        return -1;
    }

    do_clear_area(u.ux, u.uy, BOLT_LIM, openone, (genericptr_t) &num);
    return num;
}

/* callback hack for overriding vision in do_clear_area() */
boolean
detecting(func)
void FDECL((*func), (int, int, genericptr_t));
{
    return (func == findone || func == openone);
}

void
find_trap(trap)
struct trap *trap;
{
    int tt = what_trap(trap->ttyp);
    boolean cleared = FALSE;

    trap->tseen = 1;
    exercise(A_WIS, TRUE);
    feel_newsym(trap->tx, trap->ty);

    if (levl[trap->tx][trap->ty].glyph != trap_to_glyph(trap)) {
        /* There's too much clutter to see your find otherwise */
        cls();
        map_trap(trap, 1);
        display_self();
        cleared = TRUE;
    }

/*JP
    You("find %s.", an(defsyms[trap_to_defsym(tt)].explanation));
*/
    You("%s���������D", defsyms[trap_to_defsym(tt)].explanation);

    if (cleared) {
        display_nhwindow(WIN_MAP, TRUE); /* wait */
        docrt();
    }
}

STATIC_OVL int
mfind0(mtmp, via_warning)
struct monst *mtmp;
boolean via_warning;
{
    xchar x = mtmp->mx,
          y = mtmp->my;

    if (via_warning && !warning_of(mtmp))
        return -1;

    if (mtmp->m_ap_type) {
        seemimic(mtmp);
    find:
        exercise(A_WIS, TRUE);
        if (!canspotmon(mtmp)) {
            if (glyph_is_invisible(levl[x][y].glyph)) {
                /* Found invisible monster in a square which already has
                 * an 'I' in it.  Logically, this should still take time
                 * and lead to a return 1, but if we did that the player
                 * would keep finding the same monster every turn.
                 */
                return -1;
            } else {
/*JP
                                    You_feel("an unseen monster!");
*/
                                    You("�����Ȃ������̋C�z���������I");
                map_invisible(x, y);
            }
        } else if (!sensemon(mtmp))
#if 0 /*JP:T*/
                You("find %s.",
                    mtmp->mtame ? y_monnam(mtmp) : a_monnam(mtmp));
#else
                You("%s���������D",
                    mtmp->mtame ? y_monnam(mtmp) : a_monnam(mtmp));
#endif
        return 1;
    }
    if (!canspotmon(mtmp)) {
        if (mtmp->mundetected
            && (is_hider(mtmp->data) || mtmp->data->mlet == S_EEL))
            if (via_warning) {
#if 0 /*JP*/
                Your("warning senses cause you to take a second %s.",
                     Blind ? "to check nearby" : "look close by");
#else
                Your("�x���S��%s���΂炭���������D",
                     Blind ? "�܂��𒲂ׂ�̂�" : "�߂�������̂�");
#endif
                display_nhwindow(WIN_MESSAGE, FALSE); /* flush messages */
            }
        mtmp->mundetected = 0;
        newsym(x, y);
        goto find;
    }
    return 0;
}

int
dosearch0(aflag)
register int aflag; /* intrinsic autosearch vs explicit searching */
{
#ifdef GCC_BUG
    /* Some old versions of gcc seriously muck up nested loops.  If you get
     * strange crashes while searching in a version compiled with gcc, try
     * putting #define GCC_BUG in *conf.h (or adding -DGCC_BUG to CFLAGS in
     * the makefile).
     */
    volatile xchar x, y;
#else
    register xchar x, y;
#endif
    register struct trap *trap;
    register struct monst *mtmp;

    if (u.uswallow) {
        if (!aflag)
/*JP
            pline("What are you looking for?  The exit?");
*/
            pline("����T���񂾂��H�����H");
    } else {
        int fund = (uwep && uwep->oartifact
                    && spec_ability(uwep, SPFX_SEARCH)) ? uwep->spe : 0;

        if (ublindf && ublindf->otyp == LENSES && !Blind)
            fund += 2; /* JDS: lenses help searching */
        if (fund > 5)
            fund = 5;
        for (x = u.ux - 1; x < u.ux + 2; x++)
            for (y = u.uy - 1; y < u.uy + 2; y++) {
                if (!isok(x, y))
                    continue;
                if (x == u.ux && y == u.uy)
                    continue;

                if (Blind && !aflag)
                    feel_location(x, y);
                if (levl[x][y].typ == SDOOR) {
                    if (rnl(7 - fund))
                        continue;
                    cvt_sdoor_to_door(&levl[x][y]); /* .typ = DOOR */
                    exercise(A_WIS, TRUE);
                    nomul(0);
                    feel_location(x, y); /* make sure it shows up */
/*JP
                    You("find a hidden door.");
*/
                    You("�B���ꂽ�����������D");
                } else if (levl[x][y].typ == SCORR) {
                    if (rnl(7 - fund))
                        continue;
                    levl[x][y].typ = CORR;
                    unblock_point(x, y); /* vision */
                    exercise(A_WIS, TRUE);
                    nomul(0);
                    feel_newsym(x, y); /* make sure it shows up */
/*JP
                    You("find a hidden passage.");
*/
                    You("�B���ꂽ�ʘH���������D");
                } else {
                    /* Be careful not to find anything in an SCORR or SDOOR */
                    if ((mtmp = m_at(x, y)) != 0 && !aflag) {
                        int mfres = mfind0(mtmp, 0);

                        if (mfres == -1)
                            continue;
                        else if (mfres > 0)
                            return mfres;
                    }

                    /* see if an invisible monster has moved--if Blind,
                     * feel_location() already did it
                     */
                    if (!aflag && !mtmp && !Blind)
                        (void) unmap_invisible(x, y);

                    if ((trap = t_at(x, y)) && !trap->tseen && !rnl(8)) {
                        nomul(0);
                        if (trap->ttyp == STATUE_TRAP) {
                            if (activate_statue_trap(trap, x, y, FALSE))
                                exercise(A_WIS, TRUE);
                            return 1;
                        } else {
                            find_trap(trap);
                        }
                    }
                }
            }
    }
    return 1;
}

/* the 's' command -- explicit searching */
int
dosearch()
{
    return dosearch0(0);
}

void
warnreveal()
{
    int x, y;
    struct monst *mtmp;

    for (x = u.ux - 1; x <= u.ux + 1; x++)
        for (y = u.uy - 1; y <= u.uy + 1; y++) {
            if (!isok(x, y) || (x == u.ux && y == u.uy))
                continue;
            if ((mtmp = m_at(x, y)) != 0
                && warning_of(mtmp) && mtmp->mundetected)
                (void) mfind0(mtmp, 1); /* via_warning */
        }
}

/* Pre-map the sokoban levels */
void
sokoban_detect()
{
    register int x, y;
    register struct trap *ttmp;
    register struct obj *obj;

    /* Map the background and boulders */
    for (x = 1; x < COLNO; x++)
        for (y = 0; y < ROWNO; y++) {
            levl[x][y].seenv = SVALL;
            levl[x][y].waslit = TRUE;
            map_background(x, y, 1);
            if ((obj = sobj_at(BOULDER, x, y)) != 0)
                map_object(obj, 1);
        }

    /* Map the traps */
    for (ttmp = ftrap; ttmp; ttmp = ttmp->ntrap) {
        ttmp->tseen = 1;
        map_trap(ttmp, 1);
        /* set sokoban_rules when there is at least one pit or hole */
        if (ttmp->ttyp == PIT || ttmp->ttyp == HOLE)
            Sokoban = 1;
    }
}

STATIC_DCL int
reveal_terrain_getglyph(x, y, full, swallowed, default_glyph, which_subset)
int x, y, full;
unsigned swallowed;
int default_glyph, which_subset;
{
    int glyph, levl_glyph;
    uchar seenv;
    boolean keep_traps = (which_subset & TER_TRP) !=0,
            keep_objs = (which_subset & TER_OBJ) != 0,
            keep_mons = (which_subset & TER_MON) != 0;
    struct monst *mtmp;
    struct trap *t;

    /* for 'full', show the actual terrain for the entire level,
       otherwise what the hero remembers for seen locations with
       monsters, objects, and/or traps removed as caller dictates */
    seenv = (full || level.flags.hero_memory)
              ? levl[x][y].seenv : cansee(x, y) ? SVALL : 0;
    if (full) {
        levl[x][y].seenv = SVALL;
        glyph = back_to_glyph(x, y);
        levl[x][y].seenv = seenv;
    } else {
        levl_glyph = level.flags.hero_memory
              ? levl[x][y].glyph
              : seenv ? back_to_glyph(x, y): default_glyph;
        /* glyph_at() returns the displayed glyph, which might
           be a monster.  levl[][].glyph contains the remembered
           glyph, which will never be a monster (unless it is
           the invisible monster glyph, which is handled like
           an object, replacing any object or trap at its spot) */
        glyph = !swallowed ? glyph_at(x, y) : levl_glyph;
        if (keep_mons && x == u.ux && y == u.uy && swallowed)
            glyph = mon_to_glyph(u.ustuck);
        else if (((glyph_is_monster(glyph)
                   || glyph_is_warning(glyph)) && !keep_mons)
                 || glyph_is_swallow(glyph))
            glyph = levl_glyph;
        if (((glyph_is_object(glyph) && !keep_objs)
             || glyph_is_invisible(glyph))
            && keep_traps && !covers_traps(x, y)) {
            if ((t = t_at(x, y)) != 0 && t->tseen)
                glyph = trap_to_glyph(t);
        }
        if ((glyph_is_object(glyph) && !keep_objs)
            || (glyph_is_trap(glyph) && !keep_traps)
            || glyph_is_invisible(glyph)) {
            if (!seenv) {
                glyph = default_glyph;
            } else if (lastseentyp[x][y] == levl[x][y].typ) {
                glyph = back_to_glyph(x, y);
            } else {
                /* look for a mimic here posing as furniture;
                   if we don't find one, we'll have to fake it */
                if ((mtmp = m_at(x, y)) != 0
                    && mtmp->m_ap_type == M_AP_FURNITURE) {
                    glyph = cmap_to_glyph(mtmp->mappearance);
                } else {
                    /* we have a topology type but we want a screen
                       symbol in order to derive a glyph; some screen
                       symbols need the flags field of levl[][] in
                       addition to the type (to disambiguate STAIRS to
                       S_upstair or S_dnstair, for example; current
                       flags might not be intended for remembered type,
                       but we've got no other choice) */
                    schar save_typ = levl[x][y].typ;

                    levl[x][y].typ = lastseentyp[x][y];
                    glyph = back_to_glyph(x, y);
                    levl[x][y].typ = save_typ;
                }
            }
        }
    }
    if (glyph == cmap_to_glyph(S_darkroom))
        glyph = cmap_to_glyph(S_room); /* FIXME: dirty hack */
    return glyph;
}

#ifdef DUMPLOG
void
dump_map()
{
    int x, y, glyph, skippedrows, lastnonblank;
    int subset = TER_MAP | TER_TRP | TER_OBJ | TER_MON;
    int default_glyph = cmap_to_glyph(level.flags.arboreal ? S_tree : S_stone);
    char buf[BUFSZ];
    boolean blankrow, toprow;

    /*
     * Squeeze out excess vertial space when dumping the map.
     * If there are any blank map rows at the top, suppress them
     * (our caller has already printed a separator).  If there is
     * more than one blank map row at the bottom, keep just one.
     * Any blank rows within the middle of the map are kept.
     * Note: putstr() with winid==0 is for dumplog.
     */
    skippedrows = 0;
    toprow = TRUE;
    for (y = 0; y < ROWNO; y++) {
        blankrow = TRUE; /* assume blank until we discover otherwise */
        lastnonblank = -1; /* buf[] index rather than map's x */
        for (x = 1; x < COLNO; x++) {
            int ch, color;
            unsigned special;

            glyph = reveal_terrain_getglyph(x, y, FALSE, u.uswallow,
                                            default_glyph, subset);
            (void) mapglyph(glyph, &ch, &color, &special, x, y);
            buf[x - 1] = ch;
            if (ch != ' ') {
                blankrow = FALSE;
                lastnonblank = x - 1;
            }
        }
        if (!blankrow) {
            buf[lastnonblank + 1] = '\0';
            if (toprow) {
                skippedrows = 0;
                toprow = FALSE;
            }
            for (x = 0; x < skippedrows; x++)
                putstr(0, 0, "");
            putstr(0, 0, buf); /* map row #y */
            skippedrows = 0;
        } else {
            ++skippedrows;
        }
    }
    if (skippedrows)
        putstr(0, 0, "");
}
#endif /* DUMPLOG */

/* idea from crawl; show known portion of map without any monsters,
   objects, or traps occluding the view of the underlying terrain */
void
reveal_terrain(full, which_subset)
int full; /* wizard|explore modes allow player to request full map */
int which_subset; /* when not full, whether to suppress objs and/or traps */
{
    if ((Hallucination || Stunned || Confusion) && !full) {
/*JP
        You("are too disoriented for this.");
*/
        You("�������Ă���̂ł���͂ł��Ȃ��D");
    } else {
        int x, y, glyph, default_glyph;
        char buf[BUFSZ];
        /* there is a TER_MAP bit too; we always show map regardless of it */
        boolean keep_traps = (which_subset & TER_TRP) !=0,
                keep_objs = (which_subset & TER_OBJ) != 0,
                keep_mons = (which_subset & TER_MON) != 0; /* not used */
        unsigned swallowed = u.uswallow; /* before unconstrain_map() */

        if (unconstrain_map())
            docrt();
        default_glyph = cmap_to_glyph(level.flags.arboreal ? S_tree : S_stone);

        for (x = 1; x < COLNO; x++)
            for (y = 0; y < ROWNO; y++) {
                glyph = reveal_terrain_getglyph(x,y, full, swallowed,
                                                default_glyph, which_subset);
                show_glyph(x, y, glyph);
            }

        /* hero's location is not highlighted, but getpos() starts with
           cursor there, and after moving it anywhere '@' moves it back */
        flush_screen(1);
        if (full) {
/*JP
            Strcpy(buf, "underlying terrain");
*/
            Strcpy(buf, "���ɂ���n�`");
        } else {
/*JP
            Strcpy(buf, "known terrain");
*/
            Strcpy(buf, "�m���Ă���n�`");
            if (keep_traps)
#if 0 /*JP*/
                Sprintf(eos(buf), "%s traps",
                        (keep_objs || keep_mons) ? "," : " and");
#else
                Strcat(buf, "���");
#endif
            if (keep_objs)
#if 0 /*JP*/
                Sprintf(eos(buf), "%s%s objects",
                        (keep_traps || keep_mons) ? "," : "",
                        keep_mons ? "" : " and");
#else
                Strcat(buf, "�ƕ���");
#endif
            if (keep_mons)
#if 0 /*JP*/
                Sprintf(eos(buf), "%s and monsters",
                        (keep_traps || keep_objs) ? "," : "");
#else
                Strcat(buf, "�Ɖ���");
#endif
        }
/*JP
        pline("Showing %s only...", buf);
*/
        pline("%s����������D�D�D", buf);

        /* allow player to move cursor around and get autodescribe feedback
           based on what is visible now rather than what is on 'real' map */
        which_subset |= TER_MAP; /* guarantee non-zero */
        browse_map(which_subset, "anything of interest");

        reconstrain_map();
        docrt(); /* redraw the screen, restoring regular map */
        if (Underwater)
            under_water(2);
        if (u.uburied)
            under_ground(2);
    }
    return;
}

/*detect.c*/
