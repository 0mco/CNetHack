/* NetHack 3.6	shk.c	$NHDT-Date: 1515144230 2018/01/05 09:23:50 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.136 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2012. */
/* NetHack may be freely redistributed.  See license for details. */

/* JNetHack Copyright */
/* (c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994-2000  */
/* For 3.4-, Copyright (c) SHIRAKATA Kentaro, 2002-2018            */
/* JNetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#define PAY_SOME 2
#define PAY_BUY 1
#define PAY_CANT 0 /* too poor */
#define PAY_SKIP (-1)
#define PAY_BROKE (-2)

STATIC_DCL void FDECL(makekops, (coord *));
STATIC_DCL void FDECL(call_kops, (struct monst *, BOOLEAN_P));
STATIC_DCL void FDECL(kops_gone, (BOOLEAN_P));

#define NOTANGRY(mon) ((mon)->mpeaceful)
#define ANGRY(mon) (!NOTANGRY(mon))
#define IS_SHOP(x) (rooms[x].rtype >= SHOPBASE)

#define muteshk(shkp)                       \
    ((shkp)->msleeping || !(shkp)->mcanmove \
     || (shkp)->data->msound <= MS_ANIMAL)

extern const struct shclass shtypes[]; /* defined in shknam.c */

STATIC_VAR NEARDATA long int followmsg; /* last time of follow message */
/*JP
STATIC_VAR const char and_its_contents[] = " and its contents";
*/
STATIC_VAR const char and_its_contents[] = "���g�����킹��";
#if 0 /*JP*/
STATIC_VAR const char the_contents_of[] = "the contents of ";
#endif

STATIC_DCL void FDECL(append_honorific, (char *));
STATIC_DCL long FDECL(addupbill, (struct monst *));
STATIC_DCL void FDECL(pacify_shk, (struct monst *));
STATIC_DCL struct bill_x *FDECL(onbill, (struct obj *, struct monst *,
                                         BOOLEAN_P));
STATIC_DCL struct monst *FDECL(next_shkp, (struct monst *, BOOLEAN_P));
STATIC_DCL long FDECL(shop_debt, (struct eshk *));
STATIC_DCL char *FDECL(shk_owns, (char *, struct obj *));
STATIC_DCL char *FDECL(mon_owns, (char *, struct obj *));
STATIC_DCL void FDECL(clear_unpaid_obj, (struct monst *, struct obj *));
STATIC_DCL void FDECL(clear_unpaid, (struct monst *, struct obj *));
STATIC_DCL long FDECL(check_credit, (long, struct monst *));
STATIC_DCL void FDECL(pay, (long, struct monst *));
STATIC_DCL long FDECL(get_cost, (struct obj *, struct monst *));
STATIC_DCL long FDECL(set_cost, (struct obj *, struct monst *));
STATIC_DCL const char *FDECL(shk_embellish, (struct obj *, long));
STATIC_DCL long FDECL(cost_per_charge, (struct monst *, struct obj *,
                                        BOOLEAN_P));
STATIC_DCL long FDECL(cheapest_item, (struct monst *));
STATIC_DCL int FDECL(dopayobj, (struct monst *, struct bill_x *,
                                struct obj **, int, BOOLEAN_P));
STATIC_DCL long FDECL(stolen_container, (struct obj *, struct monst *,
                                         long, BOOLEAN_P));
STATIC_DCL long FDECL(getprice, (struct obj *, BOOLEAN_P));
STATIC_DCL void FDECL(shk_names_obj, (struct monst *, struct obj *,
                                      const char *, long, const char *));
STATIC_DCL struct obj *FDECL(bp_to_obj, (struct bill_x *));
STATIC_DCL boolean FDECL(inherits, (struct monst *, int, int));
STATIC_DCL void FDECL(set_repo_loc, (struct monst *));
STATIC_DCL boolean NDECL(angry_shk_exists);
STATIC_DCL void FDECL(rile_shk, (struct monst *));
STATIC_DCL void FDECL(rouse_shk, (struct monst *, BOOLEAN_P));
STATIC_DCL void FDECL(remove_damage, (struct monst *, BOOLEAN_P));
STATIC_DCL void FDECL(sub_one_frombill, (struct obj *, struct monst *));
STATIC_DCL void FDECL(add_one_tobill, (struct obj *, BOOLEAN_P,
                                       struct monst *));
STATIC_DCL void FDECL(dropped_container, (struct obj *, struct monst *,
                                          BOOLEAN_P));
STATIC_DCL void FDECL(add_to_billobjs, (struct obj *));
STATIC_DCL void FDECL(bill_box_content, (struct obj *, BOOLEAN_P, BOOLEAN_P,
                                         struct monst *));
STATIC_DCL boolean FDECL(rob_shop, (struct monst *));
STATIC_DCL void FDECL(deserted_shop, (char *));
STATIC_DCL boolean FDECL(special_stock, (struct obj *, struct monst *,
                                         BOOLEAN_P));
#if 0 /*JP*//*�g��Ȃ�*/
STATIC_DCL const char *FDECL(cad, (BOOLEAN_P));
#endif

/*
        invariants: obj->unpaid iff onbill(obj) [unless bp->useup]
                    obj->quan <= bp->bquan
 */

/*JP
static const char *angrytexts[] = { "quite upset", "ticked off", "furious" };
*/
static const char *angrytexts[] = { "���Ȃ�{����", "���𗧂Ă�", "���{����" };

/*
 *  Transfer money from inventory to monster when paying
 *  shopkeepers, priests, oracle, succubus, and other demons.
 *  Simple with only gold coins.
 *  This routine will handle money changing when multiple
 *  coin types is implemented, only appropriate
 *  monsters will pay change.  (Peaceful shopkeepers, priests
 *  and the oracle try to maintain goodwill while selling
 *  their wares or services.  Angry monsters and all demons
 *  will keep anything they get their hands on.
 *  Returns the amount actually paid, so we can know
 *  if the monster kept the change.
 */
long
money2mon(mon, amount)
struct monst *mon;
long amount;
{
    struct obj *ygold = findgold(invent);

    if (amount <= 0) {
        impossible("%s payment in money2mon!", amount ? "negative" : "zero");
        return 0L;
    }
    if (!ygold || ygold->quan < amount) {
        impossible("Paying without %s money?", ygold ? "enough" : "");
        return 0L;
    }

    if (ygold->quan > amount)
        ygold = splitobj(ygold, amount);
    else if (ygold->owornmask)
        remove_worn_item(ygold, FALSE); /* quiver */
    freeinv(ygold);
    add_to_minv(mon, ygold);
    context.botl = 1;
    return amount;
}

/*
 *  Transfer money from monster to inventory.
 *  Used when the shopkeeper pay for items, and when
 *  the priest gives you money for an ale.
 */
void
money2u(mon, amount)
struct monst *mon;
long amount;
{
    struct obj *mongold = findgold(mon->minvent);

    if (amount <= 0) {
        impossible("%s payment in money2u!", amount ? "negative" : "zero");
        return;
    }
    if (!mongold || mongold->quan < amount) {
        impossible("%s paying without %s money?", a_monnam(mon),
                   mongold ? "enough" : "");
        return;
    }

    if (mongold->quan > amount)
        mongold = splitobj(mongold, amount);
    obj_extract_self(mongold);

    if (!merge_choice(invent, mongold) && inv_cnt(FALSE) >= 52) {
/*JP
        You("have no room for the money!");
*/
        You("���������]�T���Ȃ��I");
        dropy(mongold);
    } else {
        addinv(mongold);
        context.botl = 1;
    }
}

STATIC_OVL struct monst *
next_shkp(shkp, withbill)
register struct monst *shkp;
register boolean withbill;
{
    for (; shkp; shkp = shkp->nmon) {
        if (DEADMONSTER(shkp))
            continue;
        if (shkp->isshk && (ESHK(shkp)->billct || !withbill))
            break;
    }

    if (shkp) {
        if (NOTANGRY(shkp)) {
            if (ESHK(shkp)->surcharge)
                pacify_shk(shkp);
        } else {
            if (!ESHK(shkp)->surcharge)
                rile_shk(shkp);
        }
    }
    return shkp;
}

/* called in mon.c */
void
shkgone(mtmp)
struct monst *mtmp;
{
    struct eshk *eshk = ESHK(mtmp);
    struct mkroom *sroom = &rooms[eshk->shoproom - ROOMOFFSET];
    struct obj *otmp;
    char *p;
    int sx, sy;

    /* [BUG: some of this should be done on the shop level */
    /*       even when the shk dies on a different level.] */
    if (on_level(&eshk->shoplevel, &u.uz)) {
        remove_damage(mtmp, TRUE);
        sroom->resident = (struct monst *) 0;
        if (!search_special(ANY_SHOP))
            level.flags.has_shop = 0;

        /* items on shop floor revert to ordinary objects */
        for (sx = sroom->lx; sx <= sroom->hx; sx++)
            for (sy = sroom->ly; sy <= sroom->hy; sy++)
                for (otmp = level.objects[sx][sy]; otmp;
                     otmp = otmp->nexthere)
                    otmp->no_charge = 0;

        /* Make sure bill is set only when the
           dead shk is the resident shk. */
        if ((p = index(u.ushops, eshk->shoproom)) != 0) {
            setpaid(mtmp);
            eshk->bill_p = (struct bill_x *) 0;
            /* remove eshk->shoproom from u.ushops */
            do {
                *p = *(p + 1);
            } while (*++p);
        }
    }
}

void
set_residency(shkp, zero_out)
register struct monst *shkp;
register boolean zero_out;
{
    if (on_level(&(ESHK(shkp)->shoplevel), &u.uz))
        rooms[ESHK(shkp)->shoproom - ROOMOFFSET].resident =
            (zero_out) ? (struct monst *) 0 : shkp;
}

void
replshk(mtmp, mtmp2)
register struct monst *mtmp, *mtmp2;
{
    rooms[ESHK(mtmp2)->shoproom - ROOMOFFSET].resident = mtmp2;
    if (inhishop(mtmp) && *u.ushops == ESHK(mtmp)->shoproom) {
        ESHK(mtmp2)->bill_p = &(ESHK(mtmp2)->bill[0]);
    }
}

/* do shopkeeper specific structure munging -dlc */
void
restshk(shkp, ghostly)
struct monst *shkp;
boolean ghostly;
{
    if (u.uz.dlevel) {
        struct eshk *eshkp = ESHK(shkp);

        if (eshkp->bill_p != (struct bill_x *) -1000)
            eshkp->bill_p = &eshkp->bill[0];
        /* shoplevel can change as dungeons move around */
        /* savebones guarantees that non-homed shk's will be gone */
        if (ghostly) {
            assign_level(&eshkp->shoplevel, &u.uz);
            if (ANGRY(shkp) && strncmpi(eshkp->customer, plname, PL_NSIZ))
                pacify_shk(shkp);
        }
    }
}

/* Clear the unpaid bit on a single object and its contents. */
STATIC_OVL void
clear_unpaid_obj(shkp, otmp)
struct monst *shkp;
struct obj *otmp;
{
    if (Has_contents(otmp))
        clear_unpaid(shkp, otmp->cobj);
    if (onbill(otmp, shkp, TRUE))
        otmp->unpaid = 0;
}

/* Clear the unpaid bit on all of the objects in the list. */
STATIC_OVL void
clear_unpaid(shkp, list)
struct monst *shkp;
struct obj *list;
{
    while (list) {
        clear_unpaid_obj(shkp, list);
        list = list->nobj;
    }
}

/* either you paid or left the shop or the shopkeeper died */
void
setpaid(shkp)
register struct monst *shkp;
{
    register struct obj *obj;
    register struct monst *mtmp;

    clear_unpaid(shkp, invent);
    clear_unpaid(shkp, fobj);
    clear_unpaid(shkp, level.buriedobjlist);
    if (thrownobj)
        clear_unpaid_obj(shkp, thrownobj);
    if (kickedobj)
        clear_unpaid_obj(shkp, kickedobj);
    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
        clear_unpaid(shkp, mtmp->minvent);
    for (mtmp = migrating_mons; mtmp; mtmp = mtmp->nmon)
        clear_unpaid(shkp, mtmp->minvent);

    while ((obj = billobjs) != 0) {
        obj_extract_self(obj);
        dealloc_obj(obj);
    }
    if (shkp) {
        ESHK(shkp)->billct = 0;
        ESHK(shkp)->credit = 0L;
        ESHK(shkp)->debit = 0L;
        ESHK(shkp)->loan = 0L;
    }
}

STATIC_OVL long
addupbill(shkp)
register struct monst *shkp;
{
    register int ct = ESHK(shkp)->billct;
    register struct bill_x *bp = ESHK(shkp)->bill_p;
    register long total = 0L;

    while (ct--) {
        total += bp->price * bp->bquan;
        bp++;
    }
    return total;
}

STATIC_OVL void
call_kops(shkp, nearshop)
register struct monst *shkp;
register boolean nearshop;
{
    /* Keystone Kops srt@ucla */
    register boolean nokops;

    if (!shkp)
        return;

    if (!Deaf)
/*JP
        pline("An alarm sounds!");
*/
        pline("�x�񂪖�Ђт����I");

    nokops = ((mvitals[PM_KEYSTONE_KOP].mvflags & G_GONE)
              && (mvitals[PM_KOP_SERGEANT].mvflags & G_GONE)
              && (mvitals[PM_KOP_LIEUTENANT].mvflags & G_GONE)
              && (mvitals[PM_KOP_KAPTAIN].mvflags & G_GONE));

    if (!angry_guards(!!Deaf) && nokops) {
        if (flags.verbose && !Deaf)
/*JP
            pline("But no one seems to respond to it.");
*/
            pline("�������N���������Ȃ������D");
        return;
    }

    if (nokops)
        return;

    {
        coord mm;

        if (nearshop) {
            /* Create swarm around you, if you merely "stepped out" */
            if (flags.verbose)
/*JP
                pline_The("Keystone Kops appear!");
*/
                pline("�x����������ꂽ�I");
            mm.x = u.ux;
            mm.y = u.uy;
            makekops(&mm);
            return;
        }
        if (flags.verbose)
/*JP
            pline_The("Keystone Kops are after you!");
*/
            pline("�x����������I");
        /* Create swarm near down staircase (hinders return to level) */
        mm.x = xdnstair;
        mm.y = ydnstair;
        makekops(&mm);
        /* Create swarm near shopkeeper (hinders return to shop) */
        mm.x = shkp->mx;
        mm.y = shkp->my;
        makekops(&mm);
    }
}

/* x,y is strictly inside shop */
char
inside_shop(x, y)
register xchar x, y;
{
    register char rno;

    rno = levl[x][y].roomno;
    if ((rno < ROOMOFFSET) || levl[x][y].edge || !IS_SHOP(rno - ROOMOFFSET))
        rno = NO_ROOM;
    return rno;
}

void
u_left_shop(leavestring, newlev)
char *leavestring;
boolean newlev;
{
    struct monst *shkp;
    struct eshk *eshkp;

    /*
     * IF player
     * ((didn't leave outright) AND
     *  ((he is now strictly-inside the shop) OR
     *   (he wasn't strictly-inside last turn anyway)))
     * THEN (there's nothing to do, so just return)
     */
    if (!*leavestring && (!levl[u.ux][u.uy].edge || levl[u.ux0][u.uy0].edge))
        return;

    shkp = shop_keeper(*u.ushops0);
    if (!shkp || !inhishop(shkp))
        return; /* shk died, teleported, changed levels... */

    eshkp = ESHK(shkp);
    if (!eshkp->billct && !eshkp->debit) /* bill is settled */
        return;

    if (!*leavestring && !muteshk(shkp)) {
        /*
         * Player just stepped onto shop-boundary (known from above logic).
         * Try to intimidate him into paying his bill
         */
        if (!Deaf && !muteshk(shkp))
#if 0 /*JP:T*/
            verbalize(NOTANGRY(shkp) ? "%s!  Please pay before leaving."
                                 : "%s!  Don't you leave without paying!",
                      plname);
#else
            verbalize(NOTANGRY(shkp) ? "%s����I�A��O�ɁC�����𕥂��Ă��������܂��񂩁D"
                                 : "%s�I�A��O�ɁC���𕥂��I",
                      plname);
#endif
        else
#if 0 /*JP*/
            pline("%s %s that you need to pay before leaving%s",
                Shknam(shkp),
                NOTANGRY(shkp) ? "points out" : "makes it clear",
                NOTANGRY(shkp) ? "." : "!");
#else
            pline("%s�́C�A��O�Ɏx����������K�v�����邱�Ƃ�%s",
                Shknam(shkp),
                NOTANGRY(shkp) ? "�w�E�����D" : "�͂����肳�����I");
#endif
        return;
    }

    if (rob_shop(shkp)) {
        call_kops(shkp, (!newlev && levl[u.ux0][u.uy0].edge));
    }
}

/* robbery from outside the shop via telekinesis or grappling hook */
void
remote_burglary(x, y)
xchar x, y;
{
    struct monst *shkp;
    struct eshk *eshkp;

    shkp = shop_keeper(*in_rooms(x, y, SHOPBASE));
    if (!shkp || !inhishop(shkp))
        return; /* shk died, teleported, changed levels... */

    eshkp = ESHK(shkp);
    if (!eshkp->billct && !eshkp->debit) /* bill is settled */
        return;

    if (rob_shop(shkp)) {
        /*[might want to set 2nd arg based on distance from shop doorway]*/
        call_kops(shkp, FALSE);
    }
}

/* shop merchandise has been taken; pay for it with any credit available;
   return false if the debt is fully covered by credit, true otherwise */
STATIC_OVL boolean
rob_shop(shkp)
struct monst *shkp;
{
    struct eshk *eshkp;
    long total;

    eshkp = ESHK(shkp);
    rouse_shk(shkp, TRUE);
    total = (addupbill(shkp) + eshkp->debit);
    if (eshkp->credit >= total) {
#if 0 /*JP*/
        Your("credit of %ld %s is used to cover your shopping bill.",
#else
        Your("�a��������%ld%s������̎x�����Ɏg��ꂽ�D",
#endif
             eshkp->credit, currency(eshkp->credit));
        total = 0L; /* credit gets cleared by setpaid() */
    } else {
/*JP
        You("escaped the shop without paying!");
*/
        You("���𕥂킸�ɓX���瓦�����I");
        total -= eshkp->credit;
    }
    setpaid(shkp);
    if (!total)
        return FALSE;

    /* by this point, we know an actual robbery has taken place */
    eshkp->robbed += total;
/*JP
    You("stole %ld %s worth of merchandise.", total, currency(total));
*/
    You("�G�݂�%ld%s�����񂾁D", total, currency(total));
    if (!Role_if(PM_ROGUE)) /* stealing is unlawful */
        adjalign(-sgn(u.ualign.type));

    hot_pursuit(shkp);
    return TRUE;
}

/* give a message when entering an untended shop (caller has verified that) */
STATIC_OVL void
deserted_shop(enterstring)
/*const*/ char *enterstring;
{
    struct monst *mtmp;
    struct mkroom *r = &rooms[(int) *enterstring - ROOMOFFSET];
    int x, y, m = 0, n = 0;

    for (x = r->lx; x <= r->hx; ++x)
        for (y = r->ly; y <= r->hy; ++y) {
            if (x == u.ux && y == u.uy)
                continue;
            if ((mtmp = m_at(x, y)) != 0) {
                ++n;
                if (sensemon(mtmp) || ((mtmp->m_ap_type == M_AP_NOTHING
                                        || mtmp->m_ap_type == M_AP_MONSTER)
                                       && canseemon(mtmp)))
                    ++m;
            }
        }

    if (Blind && !(Blind_telepat || Detect_monsters))
        ++n; /* force feedback to be less specific */

#if 0 /*JP*/
    pline("This shop %s %s.", (m < n) ? "seems to be" : "is",
          !n ? "deserted" : "untended");
#else
    pline("�X��%s%s�D",
          !n ? "�p�ЂƉ����Ă���" : "��������Ă���",
          (m < n) ? "�悤��" : "");
#endif
}

void
u_entered_shop(enterstring)
char *enterstring;
{
    register int rt;
    register struct monst *shkp;
    register struct eshk *eshkp;
    static char empty_shops[5];

    if (!*enterstring)
        return;

    if (!(shkp = shop_keeper(*enterstring))) {
        if (!index(empty_shops, *enterstring)
            && in_rooms(u.ux, u.uy, SHOPBASE)
                   != in_rooms(u.ux0, u.uy0, SHOPBASE))
            deserted_shop(enterstring);
        Strcpy(empty_shops, u.ushops);
        u.ushops[0] = '\0';
        return;
    }

    eshkp = ESHK(shkp);

    if (!inhishop(shkp)) {
        /* dump core when referenced */
        eshkp->bill_p = (struct bill_x *) -1000;
        if (!index(empty_shops, *enterstring))
            deserted_shop(enterstring);
        Strcpy(empty_shops, u.ushops);
        u.ushops[0] = '\0';
        return;
    }

    eshkp->bill_p = &(eshkp->bill[0]);

    if ((!eshkp->visitct || *eshkp->customer)
        && strncmpi(eshkp->customer, plname, PL_NSIZ)) {
        /* You seem to be new here */
        eshkp->visitct = 0;
        eshkp->following = 0;
        (void) strncpy(eshkp->customer, plname, PL_NSIZ);
        pacify_shk(shkp);
    }

    if (muteshk(shkp) || eshkp->following)
        return; /* no dialog */

    if (Invis) {
/*JP
        pline("%s senses your presence.", shkname(shkp));
*/
        pline("%s�͂��Ȃ��̑��݂ɋC�������D", shkname(shkp));
        if (!Deaf && !muteshk(shkp))
/*JP
        verbalize("Invisible customers are not welcome!");
*/
        verbalize("�����Ȃ��q����Ƃ͊��S���Ȃ��ȁI");
        else
#if 0 /*JP*/
            pline("%s stands firm as if %s knows you are there.",
                  Shknam(shkp), mhe(shkp));
#else
            pline("%s�́C�܂�ł��Ȃ��������ɂ��邱�Ƃ�m���Ă��邩�̂悤�ɗ����͂��������D",
                  Shknam(shkp));
#endif
        return;
    }

    rt = rooms[*enterstring - ROOMOFFSET].rtype;

    if (ANGRY(shkp)) {
        if (!Deaf && !muteshk(shkp))
#if 0 /*JP:T*/
            verbalize("So, %s, you dare return to %s %s?!", plname,
                  s_suffix(shkname(shkp)), shtypes[rt - SHOPBASE].name);
#else
            verbalize("%s�I�킴�킴%s��%s�ɖ߂��Ă����̂��H�I", plname,
                  shkname(shkp), shtypes[rt - SHOPBASE].name);
#endif
        else
#if 0 /*JP*/
            pline("%s seems %s over your return to %s %s!",
                  Shknam(shkp),
                  angrytexts[rn2(SIZE(angrytexts))],
                  mhis(shkp),
                  shtypes[rt - SHOPBASE].name);
#else
            pline("���Ȃ���%s�ɖ߂��Ă����̂�%s��%s�悤���I",
                  shtypes[rt - SHOPBASE].name,
                  Shknam(shkp),
                  angrytexts[rn2(SIZE(angrytexts))]);
#endif
    } else if (eshkp->robbed) {
        if (!Deaf)
#if 0 /*JP*/
            pline("%s mutters imprecations against shoplifters.",
                  shkname(shkp));
#else
            pline("%s�͓D�_���̂̂������D",
                  shkname(shkp));
#endif
        else
#if 0 /*JP*/
            pline("%s is combing through %s inventory list.",
                  Shknam(shkp),
                  mhis(shkp));
#else
            pline("%s�͎����̎������ꗗ���m�F���Ă���D",
                  Shknam(shkp));
#endif
    } else {
        if (!Deaf && !muteshk(shkp))
#if 0 /*JP:T*/
            verbalize("%s, %s!  Welcome%s to %s %s!", Hello(shkp), plname,
                      eshkp->visitct++ ? " again" : "",
                      s_suffix(shkname(shkp)), shtypes[rt - SHOPBASE].name);
#else
            verbalize("%s�I%s��%s��%s�I", Hello(shkp),
                      shkname(shkp), shtypes[rt - SHOPBASE].name,
                      eshkp->visitct++ ? "�܂����܂�����" : "�悤����");
#endif
        else
#if 0 /*JP*/
            You("enter %s %s%s!",
                s_suffix(shkname(shkp)),
                shtypes[rt - SHOPBASE].name,
                eshkp->visitct++ ? " again" : "");
#else
            You("%s%s��%s��K�ꂽ�I",
                eshkp->visitct++ ? "�Ă�" : "",
                shkname(shkp),
                shtypes[rt - SHOPBASE].name);
#endif
    }
    /* can't do anything about blocking if teleported in */
    if (!inside_shop(u.ux, u.uy)) {
        boolean should_block;
        int cnt;
        const char *tool;
        struct obj *pick = carrying(PICK_AXE),
                   *mattock = carrying(DWARVISH_MATTOCK);

        if (pick || mattock) {
            cnt = 1;               /* so far */
            if (pick && mattock) { /* carrying both types */
/*JP
                tool = "digging tool";
*/
                tool = "�@�铹��";
                cnt = 2; /* `more than 1' is all that matters */
            } else if (pick) {
/*JP
                tool = "pick-axe";
*/
                tool = "��͂�";
                /* hack: `pick' already points somewhere into inventory */
                while ((pick = pick->nobj) != 0)
                    if (pick->otyp == PICK_AXE)
                        ++cnt;
            } else { /* assert(mattock != 0) */
/*JP
                tool = "mattock";
*/
                tool = "��͂�";
                while ((mattock = mattock->nobj) != 0)
                    if (mattock->otyp == DWARVISH_MATTOCK)
                        ++cnt;
                /* [ALI] Shopkeeper identifies mattock(s) */
                if (!Blind)
                    makeknown(DWARVISH_MATTOCK);
            }
            if (!Deaf && !muteshk(shkp))
#if 0 /*JP:T*/
                verbalize(NOTANGRY(shkp)
                              ? "Will you please leave your %s%s outside?"
                              : "Leave the %s%s outside.",
                          tool, plur(cnt));
#else
                verbalize(NOTANGRY(shkp)
                              ? "%s���O�ɒu���Ă��Ă��������܂��񂩁H"
                              : "%s���O�֒u���Ă����I",
                          tool);
#endif
            else
#if 0 /*JP*/
                pline("%s %s to let you in with your %s%s.",
                        Shknam(shkp),
                        NOTANGRY(shkp) ? "is hesitant" : "refuses",
                        tool, plur(cnt));
#else
                pline("%s�͂��Ȃ���%s�������ē����%s�D",
                        Shknam(shkp),
                        tool,
                        NOTANGRY(shkp) ? "�͋C�������܂Ȃ��悤��" : "�����ۂ���");
#endif
            should_block = TRUE;
        } else if (u.usteed) {
            if (!Deaf && !muteshk(shkp))
#if 0 /*JP:T*/
                verbalize(NOTANGRY(shkp) ? "Will you please leave %s outside?"
                                     : "Leave %s outside.",
                      y_monnam(u.usteed));
#else
                verbalize(NOTANGRY(shkp) ? "%s���O�ɒu���Ă��Ă��������܂��񂩁H"
                                     : "%s���O�֒u���Ă����I",
                      y_monnam(u.usteed));
#endif
            else
#if 0 /*JP*/
                pline("%s %s to let you in while you're riding %s.",
                        Shknam(shkp),
                        NOTANGRY(shkp) ? "doesn't want" : "refuses",
                        y_monnam(u.usteed));
#else
                pline("%s�͂��Ȃ���%s�ɏ�����܂܂Ȃ�%s.",
                        Shknam(shkp),
                        y_monnam(u.usteed),
                        NOTANGRY(shkp) ? "�͖]�܂Ȃ��悤��" : "�����ۂ���");
#endif
            should_block = TRUE;
        } else {
            should_block =
                (Fast && (sobj_at(PICK_AXE, u.ux, u.uy)
                          || sobj_at(DWARVISH_MATTOCK, u.ux, u.uy)));
        }
        if (should_block)
            (void) dochug(shkp); /* shk gets extra move */
    }
    return;
}

/* called when removing a pick-axe or mattock from a container */
void
pick_pick(obj)
struct obj *obj;
{
    struct monst *shkp;

    if (obj->unpaid || !is_pick(obj))
        return;
    shkp = shop_keeper(*u.ushops);
    if (shkp && inhishop(shkp)) {
        static NEARDATA long pickmovetime = 0L;

        /* if you bring a sack of N picks into a shop to sell,
           don't repeat this N times when they're taken out */
        if (moves != pickmovetime) {
            if (!Deaf && !muteshk(shkp))
#if 0 /*JP*/
                verbalize("You sneaky %s!  Get out of here with that pick!",
                      cad(FALSE));
#else
                verbalize("�Ђ��傤�҂߁I��͂��������ĊO�ɏo��I");
#endif
            else
#if 0 /*JP*/
                pline("%s %s your pick!",
                      Shknam(shkp),
                      haseyes(shkp->data) ? "glares at"
                                          : "is dismayed because of");
#else
                pline("%s�͂�͂�%s�I",
                      Shknam(shkp),
                      haseyes(shkp->data) ? "���ɂ�݂���"
                                          : "�ɓ��h���Ă���");
#endif
        }
        pickmovetime = moves;
    }
}

/*
   Decide whether two unpaid items are mergable; caller is responsible for
   making sure they're unpaid and the same type of object; we check the price
   quoted by the shopkeeper and also that they both belong to the same shk.
 */
boolean
same_price(obj1, obj2)
struct obj *obj1, *obj2;
{
    register struct monst *shkp1, *shkp2;
    struct bill_x *bp1 = 0, *bp2 = 0;
    boolean are_mergable = FALSE;

    /* look up the first object by finding shk whose bill it's on */
    for (shkp1 = next_shkp(fmon, TRUE); shkp1;
         shkp1 = next_shkp(shkp1->nmon, TRUE))
        if ((bp1 = onbill(obj1, shkp1, TRUE)) != 0)
            break;
    /* second object is probably owned by same shk; if not, look harder */
    if (shkp1 && (bp2 = onbill(obj2, shkp1, TRUE)) != 0) {
        shkp2 = shkp1;
    } else {
        for (shkp2 = next_shkp(fmon, TRUE); shkp2;
             shkp2 = next_shkp(shkp2->nmon, TRUE))
            if ((bp2 = onbill(obj2, shkp2, TRUE)) != 0)
                break;
    }

    if (!bp1 || !bp2)
        impossible("same_price: object wasn't on any bill!");
    else
        are_mergable = (shkp1 == shkp2 && bp1->price == bp2->price);
    return are_mergable;
}

/*
 * Figure out how much is owed to a given shopkeeper.
 * At present, we ignore any amount robbed from the shop, to avoid
 * turning the `$' command into a way to discover that the current
 * level is bones data which has a shk on the warpath.
 */
STATIC_OVL long
shop_debt(eshkp)
struct eshk *eshkp;
{
    struct bill_x *bp;
    int ct;
    long debt = eshkp->debit;

    for (bp = eshkp->bill_p, ct = eshkp->billct; ct > 0; bp++, ct--)
        debt += bp->price * bp->bquan;
    return debt;
}

/* called in response to the `$' command */
void
shopper_financial_report()
{
    struct monst *shkp, *this_shkp = shop_keeper(inside_shop(u.ux, u.uy));
    struct eshk *eshkp;
    long amt;
    int pass;

    eshkp = this_shkp ? ESHK(this_shkp) : 0;
    if (eshkp && !(eshkp->credit || shop_debt(eshkp))) {
/*JP
        You("have no credit or debt in here.");
*/
        You("�a�������؋����Ȃ��D");
        this_shkp = 0; /* skip first pass */
    }

    /* pass 0: report for the shop we're currently in, if any;
       pass 1: report for all other shops on this level. */
    for (pass = this_shkp ? 0 : 1; pass <= 1; pass++)
        for (shkp = next_shkp(fmon, FALSE); shkp;
             shkp = next_shkp(shkp->nmon, FALSE)) {
            if ((shkp != this_shkp) ^ pass)
                continue;
            eshkp = ESHK(shkp);
            if ((amt = eshkp->credit) != 0)
#if 0 /*JP*/
                You("have %ld %s credit at %s %s.", amt, currency(amt),
                    s_suffix(shkname(shkp)),
                    shtypes[eshkp->shoptype - SHOPBASE].name);
#else
                You("%ld%s�̗a������%s��%s�ɂ���D", amt, currency(amt),
                    shkname(shkp),
                    shtypes[eshkp->shoptype - SHOPBASE].name);
#endif
            else if (shkp == this_shkp)
/*JP
                You("have no credit in here.");
*/
                You("�a�����͂Ȃ��D");
            if ((amt = shop_debt(eshkp)) != 0)
/*JP
                You("owe %s %ld %s.", shkname(shkp), amt, currency(amt));
*/
                You("%s��%ld%s�̎؂肪����D", shkname(shkp), amt, currency(amt));
            else if (shkp == this_shkp)
/*JP
                You("don't owe any money here.");
*/
                pline("���̓X�Ɏ؂�͂Ȃ��D");
        }
}

int
inhishop(mtmp)
register struct monst *mtmp;
{
    struct eshk *eshkp = ESHK(mtmp);

    return (index(in_rooms(mtmp->mx, mtmp->my, SHOPBASE), eshkp->shoproom)
            && on_level(&eshkp->shoplevel, &u.uz));
}

struct monst *
shop_keeper(rmno)
char rmno;
{
    struct monst *shkp;

    shkp = (rmno >= ROOMOFFSET) ? rooms[rmno - ROOMOFFSET].resident : 0;
    if (shkp) {
        if (has_eshk(shkp)) {
            if (NOTANGRY(shkp)) {
                if (ESHK(shkp)->surcharge)
                    pacify_shk(shkp);
            } else {
                if (!ESHK(shkp)->surcharge)
                    rile_shk(shkp);
            }
        } else {
            /* would have segfaulted on ESHK dereference previously */
            impossible("%s? (rmno=%d, rtype=%d, mnum=%d, \"%s\")",
                       shkp->isshk ? "shopkeeper career change"
                                   : "shop resident not shopkeeper",
                       (int) rmno,
                       (int) rooms[rmno - ROOMOFFSET].rtype,
                       shkp->mnum,
                       /* [real shopkeeper name is kept in ESHK, not MNAME] */
                       has_mname(shkp) ? MNAME(shkp) : "anonymous");
            /* not sure if this is appropriate, because it does nothing to
               correct the underlying rooms[].resident issue but... */
            return (struct monst *) 0;
        }
    }
    return shkp;
}

boolean
tended_shop(sroom)
struct mkroom *sroom;
{
    struct monst *mtmp = sroom->resident;

    return !mtmp ? FALSE : (boolean) inhishop(mtmp);
}

STATIC_OVL struct bill_x *
onbill(obj, shkp, silent)
struct obj *obj;
struct monst *shkp;
boolean silent;
{
    if (shkp) {
        register struct bill_x *bp = ESHK(shkp)->bill_p;
        register int ct = ESHK(shkp)->billct;

        while (--ct >= 0)
            if (bp->bo_id == obj->o_id) {
                if (!obj->unpaid)
/*JP
                    pline("onbill: paid obj on bill?");
*/
                    pline("����F�����H");
                return bp;
            } else
                bp++;
    }
    if (obj->unpaid && !silent)
/*JP
        pline("onbill: unpaid obj not on bill?");
*/
        pline("����F����Ȃ��H");
    return (struct bill_x *) 0;
}

/* check whether an object or any of its contents belongs to a shop */
boolean
is_unpaid(obj)
struct obj *obj;
{
    return (boolean) (obj->unpaid
                      || (Has_contents(obj) && count_unpaid(obj->cobj)));
}

/* Delete the contents of the given object. */
void
delete_contents(obj)
register struct obj *obj;
{
    register struct obj *curr;

    while ((curr = obj->cobj) != 0) {
        obj_extract_self(curr);
        obfree(curr, (struct obj *) 0);
    }
}

/* called with two args on merge */
void
obfree(obj, merge)
register struct obj *obj, *merge;
{
    register struct bill_x *bp;
    register struct bill_x *bpm;
    register struct monst *shkp;

    if (obj->otyp == LEASH && obj->leashmon)
        o_unleash(obj);
    if (obj->oclass == FOOD_CLASS)
        food_disappears(obj);
    if (obj->oclass == SPBOOK_CLASS)
        book_disappears(obj);
    if (Has_contents(obj))
        delete_contents(obj);

    shkp = 0;
    if (obj->unpaid) {
        /* look for a shopkeeper who owns this object */
        for (shkp = next_shkp(fmon, TRUE); shkp;
             shkp = next_shkp(shkp->nmon, TRUE))
            if (onbill(obj, shkp, TRUE))
                break;
    }
    /* sanity check, more or less */
    if (!shkp)
        shkp = shop_keeper(*u.ushops);
    /*
     * Note:  `shkp = shop_keeper(*u.ushops)' used to be
     *    unconditional.  But obfree() is used all over
     *    the place, so making its behavior be dependent
     *    upon player location doesn't make much sense.
     */

    if ((bp = onbill(obj, shkp, FALSE)) != 0) {
        if (!merge) {
            bp->useup = 1;
            obj->unpaid = 0; /* only for doinvbill */
            add_to_billobjs(obj);
            return;
        }
        bpm = onbill(merge, shkp, FALSE);
        if (!bpm) {
            /* this used to be a rename */
            impossible("obfree: not on bill??");
            return;
        } else {
            /* this was a merger */
            bpm->bquan += bp->bquan;
            ESHK(shkp)->billct--;
#ifdef DUMB
            {
                /* DRS/NS 2.2.6 messes up -- Peter Kendell */
                int indx = ESHK(shkp)->billct;

                *bp = ESHK(shkp)->bill_p[indx];
            }
#else
            *bp = ESHK(shkp)->bill_p[ESHK(shkp)->billct];
#endif
        }
    }
    if (obj->owornmask) {
        impossible("obfree: deleting worn obj (%d: %ld)", obj->otyp,
                   obj->owornmask);
        /* unfortunately at this point we don't know whether worn mask
           applied to hero or a monster or perhaps something bogus, so
           can't call remove_worn_item() to get <X>_off() side-effects */
        setnotworn(obj);
    }
    dealloc_obj(obj);
}

STATIC_OVL long
check_credit(tmp, shkp)
long tmp;
register struct monst *shkp;
{
    long credit = ESHK(shkp)->credit;

    if (credit == 0L) {
        ; /* nothing to do; just 'return tmp;' */
    } else if (credit >= tmp) {
/*JP
        pline_The("price is deducted from your credit.");
*/
        pline("����͗a�������獷�������ꂽ�D");
        ESHK(shkp)->credit -= tmp;
        tmp = 0L;
    } else {
/*JP
        pline_The("price is partially covered by your credit.");
*/
        pline("����̈ꕔ�͂��Ȃ��̗a�����ŕ��ꂽ�D");
        ESHK(shkp)->credit = 0L;
        tmp -= credit;
    }
    return tmp;
}

STATIC_OVL void
pay(tmp, shkp)
long tmp;
register struct monst *shkp;
{
    long robbed = ESHK(shkp)->robbed;
    long balance = ((tmp <= 0L) ? tmp : check_credit(tmp, shkp));

    if (balance > 0)
        money2mon(shkp, balance);
    else if (balance < 0)
        money2u(shkp, -balance);
    context.botl = 1;
    if (robbed) {
        robbed -= tmp;
        if (robbed < 0)
            robbed = 0L;
        ESHK(shkp)->robbed = robbed;
    }
}

/* return shkp to home position */
void
home_shk(shkp, killkops)
register struct monst *shkp;
register boolean killkops;
{
    register xchar x = ESHK(shkp)->shk.x, y = ESHK(shkp)->shk.y;

    (void) mnearto(shkp, x, y, TRUE);
    level.flags.has_shop = 1;
    if (killkops) {
        kops_gone(TRUE);
        pacify_guards();
    }
    after_shk_move(shkp);
}

STATIC_OVL boolean
angry_shk_exists()
{
    register struct monst *shkp;

    for (shkp = next_shkp(fmon, FALSE); shkp;
         shkp = next_shkp(shkp->nmon, FALSE))
        if (ANGRY(shkp))
            return TRUE;
    return FALSE;
}

/* remove previously applied surcharge from all billed items */
STATIC_OVL void
pacify_shk(shkp)
register struct monst *shkp;
{
    NOTANGRY(shkp) = TRUE; /* make peaceful */
    if (ESHK(shkp)->surcharge) {
        register struct bill_x *bp = ESHK(shkp)->bill_p;
        register int ct = ESHK(shkp)->billct;

        ESHK(shkp)->surcharge = FALSE;
        while (ct-- > 0) {
            register long reduction = (bp->price + 3L) / 4L;
            bp->price -= reduction; /* undo 33% increase */
            bp++;
        }
    }
}

/* add aggravation surcharge to all billed items */
STATIC_OVL void
rile_shk(shkp)
register struct monst *shkp;
{
    NOTANGRY(shkp) = FALSE; /* make angry */
    if (!ESHK(shkp)->surcharge) {
        register struct bill_x *bp = ESHK(shkp)->bill_p;
        register int ct = ESHK(shkp)->billct;

        ESHK(shkp)->surcharge = TRUE;
        while (ct-- > 0) {
            register long surcharge = (bp->price + 2L) / 3L;
            bp->price += surcharge;
            bp++;
        }
    }
}

/* wakeup and/or unparalyze shopkeeper */
STATIC_OVL void
rouse_shk(shkp, verbosely)
struct monst *shkp;
boolean verbosely;
{
    if (!shkp->mcanmove || shkp->msleeping) {
        /* greed induced recovery... */
        if (verbosely && canspotmon(shkp))
#if 0 /*JP:T*/
            pline("%s %s.", Shknam(shkp),
                  shkp->msleeping ? "wakes up" : "can move again");
#else
            pline("%s��%s�D", Monnam(shkp),
                  shkp->msleeping ? "�ڂ��o�߂�" : "�ӂ����ѓ�����悤�ɂȂ���");
#endif
        shkp->msleeping = 0;
        shkp->mfrozen = 0;
        shkp->mcanmove = 1;
    }
}

void
make_happy_shk(shkp, silentkops)
register struct monst *shkp;
register boolean silentkops;
{
    boolean wasmad = ANGRY(shkp);
    struct eshk *eshkp = ESHK(shkp);

    pacify_shk(shkp);
    eshkp->following = 0;
    eshkp->robbed = 0L;
    if (!Role_if(PM_ROGUE))
        adjalign(sgn(u.ualign.type));
    if (!inhishop(shkp)) {
        char shk_nam[BUFSZ];
        boolean vanished = canseemon(shkp);

        Strcpy(shk_nam, shkname(shkp));
        if (on_level(&eshkp->shoplevel, &u.uz)) {
            home_shk(shkp, FALSE);
            /* didn't disappear if shk can still be seen */
            if (canseemon(shkp))
                vanished = FALSE;
        } else {
            /* if sensed, does disappear regardless whether seen */
            if (sensemon(shkp))
                vanished = TRUE;
            /* can't act as porter for the Amulet, even if shk
               happens to be going farther down rather than up */
            mdrop_special_objs(shkp);
            /* arrive near shop's door */
            migrate_to_level(shkp, ledger_no(&eshkp->shoplevel),
                             MIGR_APPROX_XY, &eshkp->shd);
            /* dismiss kops on that level when shk arrives */
            eshkp->dismiss_kops = TRUE;
        }
        if (vanished)
/*JP
            pline("Satisfied, %s suddenly disappears!", shk_nam);
*/
            pline("%s�͖�������ƁC�ˑR�������I", shk_nam);
    } else if (wasmad)
/*JP
        pline("%s calms down.", Shknam(shkp));
*/
        pline("%s�͗��������D", Shknam(shkp));

    make_happy_shoppers(silentkops);
}

/* called by make_happy_shk() and also by losedogs() for migrating shk */
void
make_happy_shoppers(silentkops)
boolean silentkops;
{
    if (!angry_shk_exists()) {
        kops_gone(silentkops);
        pacify_guards();
    }
}

void
hot_pursuit(shkp)
register struct monst *shkp;
{
    if (!shkp->isshk)
        return;

    rile_shk(shkp);
    (void) strncpy(ESHK(shkp)->customer, plname, PL_NSIZ);
    ESHK(shkp)->following = 1;
}

/* Used when the shkp is teleported or falls (ox == 0) out of his shop, or
   when the player is not on a costly_spot and he damages something inside
   the shop.  These conditions must be checked by the calling function. */
/*ARGSUSED*/
void
make_angry_shk(shkp, ox, oy)
struct monst *shkp;
xchar ox UNUSED; /* <ox,oy> predate 'noit_Monnam()', let alone Shknam() */
xchar oy UNUSED;
{
    struct eshk *eshkp = ESHK(shkp);

    /* all pending shop transactions are now "past due" */
    if (eshkp->billct || eshkp->debit || eshkp->loan || eshkp->credit) {
        eshkp->robbed += (addupbill(shkp) + eshkp->debit + eshkp->loan);
        eshkp->robbed -= eshkp->credit;
        if (eshkp->robbed < 0L)
            eshkp->robbed = 0L;
        /* billct, debit, loan, and credit will be cleared by setpaid */
        setpaid(shkp);
    }

/*JP
    pline("%s %s!", Shknam(shkp), !ANGRY(shkp) ? "gets angry" : "is furious");
*/
    pline("%s��%s�I", Shknam(shkp), !ANGRY(shkp) ? "�{����" : "�{�苶����");
    hot_pursuit(shkp);
}

STATIC_VAR const char
/*JP
        no_money[] = "Moreover, you%s have no money.",
*/
        no_money[] = "�������C���Ȃ��͂������Ȃ�%s�D",
/*JP
        not_enough_money[] = "Besides, you don't have enough to interest %s.";
*/
        not_enough_money[] = "�������C���Ȃ���%s�����������قǂ����������Ă��Ȃ��I";

/* delivers the cheapest item on the list */
STATIC_OVL long
cheapest_item(shkp)
register struct monst *shkp;
{
    register int ct = ESHK(shkp)->billct;
    register struct bill_x *bp = ESHK(shkp)->bill_p;
    register long gmin = (bp->price * bp->bquan);

    while (ct--) {
        if (bp->price * bp->bquan < gmin)
            gmin = bp->price * bp->bquan;
        bp++;
    }
    return gmin;
}

int
dopay()
{
    register struct eshk *eshkp;
    register struct monst *shkp;
    struct monst *nxtm, *resident;
    long ltmp;
    long umoney;
    int pass, tmp, sk = 0, seensk = 0;
    boolean paid = FALSE, stashed_gold = (hidden_gold() > 0L);

    multi = 0;

    /* Find how many shk's there are, how many are in
     * sight, and are you in a shop room with one.
     */
    nxtm = resident = 0;
    for (shkp = next_shkp(fmon, FALSE); shkp;
         shkp = next_shkp(shkp->nmon, FALSE)) {
        sk++;
        if (ANGRY(shkp) && distu(shkp->mx, shkp->my) <= 2)
            nxtm = shkp;
        if (canspotmon(shkp))
            seensk++;
        if (inhishop(shkp) && (*u.ushops == ESHK(shkp)->shoproom))
            resident = shkp;
    }

    if (nxtm) {      /* Player should always appease an */
        shkp = nxtm; /* irate shk standing next to them. */
        goto proceed;
    }

    if ((!sk && (!Blind || Blind_telepat)) || (!Blind && !seensk)) {
/*JP
        There("appears to be no shopkeeper here to receive your payment.");
*/
        pline("�x�������󂯂Ƃ�X��͂����ɂ͂��Ȃ��悤���D");
        return 0;
    }

    if (!seensk) {
/*JP
        You_cant("see...");
*/
        You("���邱�Ƃ��ł��Ȃ��D�D�D");
        return 0;
    }

    /* The usual case.  Allow paying at a distance when
     * inside a tended shop.  Should we change that?
     */
    if (sk == 1 && resident) {
        shkp = resident;
        goto proceed;
    }

    if (seensk == 1) {
        for (shkp = next_shkp(fmon, FALSE); shkp;
             shkp = next_shkp(shkp->nmon, FALSE))
            if (canspotmon(shkp))
                break;
        if (shkp != resident && distu(shkp->mx, shkp->my) > 2) {
/*JP
            pline("%s is not near enough to receive your payment.",
*/
            pline("%s�͉����ɂ���̂Ŏx�����Ȃ��D",
                  Shknam(shkp));
            return 0;
        }
    } else {
        struct monst *mtmp;
        coord cc;
        int cx, cy;

/*JP
        pline("Pay whom?");
*/
        pline("�N�ɕ����H");
        cc.x = u.ux;
        cc.y = u.uy;
/*JP
        if (getpos(&cc, TRUE, "the creature you want to pay") < 0)
*/
        if (getpos(&cc, TRUE, "�x������������") < 0)
            return 0; /* player pressed ESC */
        cx = cc.x;
        cy = cc.y;
        if (cx < 0) {
/*JP
            pline("Try again...");
*/
            pline("������x�D�D�D");
            return 0;
        }
        if (u.ux == cx && u.uy == cy) {
/*JP
            You("are generous to yourself.");
*/
            pline("�������g�ɂȂ�ċC�O�̂������ƁI");
            return 0;
        }
        mtmp = m_at(cx, cy);
        if (!cansee(cx, cy) && (!mtmp || !canspotmon(mtmp))) {
            You("can't %s anyone there.", !Blind ? "see" : "sense");
            return 0;
        }
        if (!mtmp) {
/*JP
            There("is no one there to receive your payment.");
*/
            pline("�x�������󂯂Ƃ�鑊��͂��Ȃ��D");
            return 0;
        }
        if (!mtmp->isshk) {
/*JP
            pline("%s is not interested in your payment.", Monnam(mtmp));
*/
            pline("%s�͎x�����ɋ����������Ȃ��D", Monnam(mtmp));
            return 0;
        }
        if (mtmp != resident && distu(mtmp->mx, mtmp->my) > 2) {
/*JP
            pline("%s is too far to receive your payment.", Shknam(mtmp));
*/
            pline("%s�͎x�����ɋ����������Ȃ��D", Shknam(mtmp));
            return 0;
        }
        shkp = mtmp;
    }

    if (!shkp) {
        debugpline0("dopay: null shkp.");
        return 0;
    }
proceed:
    eshkp = ESHK(shkp);
    ltmp = eshkp->robbed;

    /* wake sleeping shk when someone who owes money offers payment */
    if (ltmp || eshkp->billct || eshkp->debit)
        rouse_shk(shkp, TRUE);

    if (!shkp->mcanmove || shkp->msleeping) { /* still asleep/paralyzed */
#if 0 /*JP:T*/
        pline("%s %s.", Shknam(shkp),
              rn2(2) ? "seems to be napping" : "doesn't respond");
#else
        pline("%s��%s�D", Shknam(shkp),
              rn2(2) ? "����������Ă���悤��" : "�������Ȃ�");
#endif
        return 0;
    }

    if (shkp != resident && NOTANGRY(shkp)) {
        umoney = money_cnt(invent);
        if (!ltmp)
/*JP
            You("do not owe %s anything.", shkname(shkp));
*/
            You("%s�Ɏ؂�͂Ȃ��D", shkname(shkp));
        else if (!umoney) {
/*JP
            You("%shave no money.", stashed_gold ? "seem to " : "");
*/
            You("�������Ȃ�%s�D", stashed_gold ? "�悤��" : "");
            if (stashed_gold)
/*JP
                pline("But you have some gold stashed away.");
*/
                pline("�������C���Ȃ��ɂ͂�������Ƃ����w�\�N��������D");
        } else {
            if (umoney > ltmp) {
#if 0 /*JP*/
                You("give %s the %ld gold piece%s %s asked for.",
                    shkname(shkp), ltmp, plur(ltmp), mhe(shkp));
#else
                You("%s�ɖ]�ݒʂ�%ld���̋��݂�^�����D",
                    shkname(shkp), ltmp);
#endif
                pay(ltmp, shkp);
            } else {
#if 0 /*JP*/
                You("give %s all your%s gold.", shkname(shkp),
                    stashed_gold ? " openly kept" : "");
#else
                You("%s��%s�����S����^�����D", shkname(shkp),
                    stashed_gold ? "�莝����" : "");
#endif
                pay(umoney, shkp);
                if (stashed_gold)
/*JP
                    pline("But you have hidden gold!");
*/
                    pline("�������C���Ȃ��̓w�\�N��������I");
            }
            if ((umoney < ltmp / 2L) || (umoney < ltmp && stashed_gold))
/*JP
                pline("Unfortunately, %s doesn't look satisfied.", mhe(shkp));
*/
                pline("�c�O�Ȃ���C%s�͖������ĂȂ��悤���D", mhe(shkp));
            else
                make_happy_shk(shkp, FALSE);
        }
        return 1;
    }

    /* ltmp is still eshkp->robbed here */
    if (!eshkp->billct && !eshkp->debit) {
        umoney = money_cnt(invent);
        if (!ltmp && NOTANGRY(shkp)) {
/*JP
            You("do not owe %s anything.", shkname(shkp));
*/
            You("%s�Ɏ؂�͂Ȃ��D", shkname(shkp));
            if (!umoney)
/*JP
                pline(no_money, stashed_gold ? " seem to" : "");
*/
                pline(no_money, stashed_gold ? "�悤��" : "");
        } else if (ltmp) {
/*JP
            pline("%s is after blood, not money!", shkname(shkp));
*/
            pline("%s�͌��܂݂ꂾ�D�����ǂ��낶��Ȃ��I", Monnam(shkp));
            if (umoney < ltmp / 2L || (umoney < ltmp && stashed_gold)) {
                if (!umoney)
/*JP
                    pline(no_money, stashed_gold ? " seem to" : "");
*/
                    pline(no_money, stashed_gold ? "�悤��" : "");
                else
                    pline(not_enough_money, mhim(shkp));
                return 1;
            }
/*JP
            pline("But since %s shop has been robbed recently,", mhis(shkp));
*/
            pline("�������C%s�X�͍ŋߓ��݂ɂ������̂ŁC", mhis(shkp));
#if 0 /*JP*/
            pline("you %scompensate %s for %s losses.",
                  (umoney < ltmp) ? "partially " : "", shkname(shkp),
                  mhis(shkp));
#else
            pline("%s�̑���%s���U�����D", mhis(shkp),
                  (umoney < ltmp) ? "�̈ꕔ" : "");
#endif
            pay(umoney < ltmp ? umoney : ltmp, shkp);
            make_happy_shk(shkp, FALSE);
        } else {
            /* shopkeeper is angry, but has not been robbed --
             * door broken, attacked, etc. */
/*JP
            pline("%s is after your hide, not your money!", Shknam(shkp));
*/
            pline("%s�͂��Ȃ��̖���_���Ă���C�����ǂ��낶��Ȃ��I", Shknam(shkp));
            if (umoney < 1000L) {
                if (!umoney)
/*JP
                    pline(no_money, stashed_gold ? " seem to" : "");
*/
                    pline(no_money, stashed_gold ? "�悤��" : "");
                else
                    pline(not_enough_money, mhim(shkp));
                return 1;
            }
#if 0 /*JP*/
            You("try to appease %s by giving %s 1000 gold pieces.",
                x_monnam(shkp, ARTICLE_THE, "angry", 0, FALSE), mhim(shkp));
#else
            You("����1000����%s�Ɏ�n���ĂȂ��߂悤�Ƃ����D",
                x_monnam(shkp, ARTICLE_THE, "�{����", 0, FALSE));
#endif
            pay(1000L, shkp);
            if (strncmp(eshkp->customer, plname, PL_NSIZ) || rn2(3))
                make_happy_shk(shkp, FALSE);
            else
/*JP
                pline("But %s is as angry as ever.", shkname(shkp));
*/
                pline("������%s�͂܂��{���Ă���D", shkname(shkp));
        }
        return 1;
    }
    if (shkp != resident) {
        impossible("dopay: not to shopkeeper?");
        if (resident)
            setpaid(resident);
        return 0;
    }
    /* pay debt, if any, first */
    if (eshkp->debit) {
        long dtmp = eshkp->debit;
        long loan = eshkp->loan;
        char sbuf[BUFSZ];

        umoney = money_cnt(invent);
#if 0 /*JP*//*�����ɉ�*/
        Sprintf(sbuf, "You owe %s %ld %s ", shkname(shkp), dtmp,
                currency(dtmp));
#endif
        if (loan) {
            if (loan == dtmp)
/*JP
                Strcat(sbuf, "you picked up in the store.");
*/
                        Strcpy(sbuf, "�X�̒��ŏE�������̂ɑ΂��āC");
            else
#if 0 /*JP*/
                Strcat(sbuf,
                       "for gold picked up and the use of merchandise.");
#else
                Strcpy(sbuf,"�E����������g�����G�݂ɑ΂��āC");
#endif
        } else
/*JP
            Strcat(sbuf, "for the use of merchandise.");
*/
            Strcpy(sbuf, "�g�����G�݂ɑ΂��āC");
#if 1 /*JP*//*�ォ��ړ�*/
        Sprintf(eos(sbuf), "%s��%ld%s�̎؂肪����D",
                shkname(shkp), dtmp, currency(dtmp));
#endif
        pline1(sbuf);
        if (umoney + eshkp->credit < dtmp) {
#if 0 /*JP*/
            pline("But you don't%s have enough gold%s.",
                  stashed_gold ? " seem to" : "",
                  eshkp->credit ? " or credit" : "");
#else
            pline("�������C����%s����Ȃ�%s�D",
                  eshkp->credit ? "���a������" : "��",
                  stashed_gold ? "�悤��" : "");
#endif
            return 1;
        } else {
            if (eshkp->credit >= dtmp) {
                eshkp->credit -= dtmp;
                eshkp->debit = 0L;
                eshkp->loan = 0L;
/*JP
                Your("debt is covered by your credit.");
*/
                Your("�؋��͗a�����ŕ��ꂽ�D");
            } else if (!eshkp->credit) {
                money2mon(shkp, dtmp);
                eshkp->debit = 0L;
                eshkp->loan = 0L;
/*JP
                You("pay that debt.");
*/
                You("�؋��𕥂����D");
                context.botl = 1;
            } else {
                dtmp -= eshkp->credit;
                eshkp->credit = 0L;
                money2mon(shkp, dtmp);
                eshkp->debit = 0L;
                eshkp->loan = 0L;
/*JP
                pline("That debt is partially offset by your credit.");
*/
                pline("���̎؋��͈ꕔ�a�����ő��E���ꂽ�D");
/*JP
                You("pay the remainder.");
*/
                You("�c��𕥂����D");
                context.botl = 1;
            }
            paid = TRUE;
        }
    }
    /* now check items on bill */
    if (eshkp->billct) {
        register boolean itemize;
        int iprompt;

        umoney = money_cnt(invent);
        if (!umoney && !eshkp->credit) {
#if 0 /*JP*/
            You("%shave no money or credit%s.",
                stashed_gold ? "seem to " : "", paid ? " left" : "");
#else
            You("%s�������a�����������ĂȂ�%s�D",
                paid ? "����" : "", stashed_gold ? "�悤��" : "");
#endif
            return 0;
        }
        if ((umoney + eshkp->credit) < cheapest_item(shkp)) {
#if 0 /*JP*/
            You("don't have enough money to buy%s the item%s you picked.",
                eshkp->billct > 1 ? " any of" : "", plur(eshkp->billct));
#else
            pline("�E�����i���𔃂��ɂ͂���������Ȃ��D");
#endif
            if (stashed_gold)
/*JP
                pline("Maybe you have some gold stashed away?");
*/
                You("�ǂ����ɂ������B���Ă���̂����H");
            return 0;
        }

        /* this isn't quite right; it itemizes without asking if the
         * single item on the bill is partly used up and partly unpaid */
/*JP
        iprompt = (eshkp->billct > 1 ? ynq("Itemized billing?") : 'y');
*/
        iprompt = (eshkp->billct > 1 ? ynq("�ʂɊ��肵�܂����H") : 'y');
        itemize = (iprompt == 'y');
        if (iprompt == 'q')
            goto thanks;

        for (pass = 0; pass <= 1; pass++) {
            tmp = 0;
            while (tmp < eshkp->billct) {
                struct obj *otmp;
                register struct bill_x *bp = &(eshkp->bill_p[tmp]);

                /* find the object on one of the lists */
                if ((otmp = bp_to_obj(bp)) != 0) {
                    /* if completely used up, object quantity is stale;
                       restoring it to its original value here avoids
                       making the partly-used-up code more complicated */
                    if (bp->useup)
                        otmp->quan = bp->bquan;
                } else {
                    impossible("Shopkeeper administration out of order.");
                    setpaid(shkp); /* be nice to the player */
                    return 1;
                }
                if (pass == bp->useup && otmp->quan == bp->bquan) {
                    /* pay for used-up items on first pass and others
                     * on second, so player will be stuck in the store
                     * less often; things which are partly used up
                     * are processed on both passes */
                    tmp++;
                } else {
                    switch (dopayobj(shkp, bp, &otmp, pass, itemize)) {
                    case PAY_CANT:
                        return 1; /*break*/
                    case PAY_BROKE:
                        paid = TRUE;
                        goto thanks; /*break*/
                    case PAY_SKIP:
                        tmp++;
                        continue; /*break*/
                    case PAY_SOME:
                        paid = TRUE;
                        if (itemize)
                            bot();
                        continue; /*break*/
                    case PAY_BUY:
                        paid = TRUE;
                        break;
                    }
                    if (itemize)
                        bot();
                    *bp = eshkp->bill_p[--eshkp->billct];
                }
            }
        }
    thanks:
        if (!itemize)
            update_inventory(); /* Done in dopayobj() if itemize. */
    }
    if (!ANGRY(shkp) && paid) {
        if (!Deaf && !muteshk(shkp))
#if 0 /*JP*/
            verbalize("Thank you for shopping in %s %s!", s_suffix(shkname(shkp)),
                  shtypes[eshkp->shoptype - SHOPBASE].name);
#else
            verbalize("%s��%s�ւ܂��ǂ����I", shkname(shkp),
                  shtypes[eshkp->shoptype - SHOPBASE].name);
#endif
        else
#if 0 /*JP*/
            pline("%s nods appreciatively at you for shopping in %s %s!",
                    Shknam(shkp),
                    mhis(shkp),
                    shtypes[eshkp->shoptype - SHOPBASE].name);
#else
            pline("%s��%s�ł̔������Ɋ��ӂ��Ă��ȂÂ����I",
                    Shknam(shkp),
                    shtypes[eshkp->shoptype - SHOPBASE].name);
#endif
    }
    return 1;
}

/* return 2 if used-up portion paid
 *        1 if paid successfully
 *        0 if not enough money
 *       -1 if skip this object
 *       -2 if no money/credit left
 */
STATIC_OVL int
dopayobj(shkp, bp, obj_p, which, itemize)
register struct monst *shkp;
register struct bill_x *bp;
struct obj **obj_p;
int which; /* 0 => used-up item, 1 => other (unpaid or lost) */
boolean itemize;
{
    register struct obj *obj = *obj_p;
    long ltmp, quan, save_quan;
    long umoney = money_cnt(invent);
    int buy;
    boolean stashed_gold = (hidden_gold() > 0L), consumed = (which == 0);

    if (!obj->unpaid && !bp->useup) {
        impossible("Paid object on bill??");
        return PAY_BUY;
    }
    if (itemize && umoney + ESHK(shkp)->credit == 0L) {
#if 0 /*JP*/
        You("%shave no money or credit left.",
            stashed_gold ? "seem to " : "");
#else
        You("�����������a�������Ȃ�%s�D",
            stashed_gold ? "�悤��" : "");
#endif
        return PAY_BROKE;
    }
    /* we may need to temporarily adjust the object, if part of the
       original quantity has been used up but part remains unpaid  */
    save_quan = obj->quan;
    if (consumed) {
        /* either completely used up (simple), or split needed */
        quan = bp->bquan;
        if (quan > obj->quan) /* difference is amount used up */
            quan -= obj->quan;
    } else {
        /* dealing with ordinary unpaid item */
        quan = obj->quan;
    }
    obj->quan = quan;        /* to be used by doname() */
    obj->unpaid = 0;         /* ditto */
    iflags.suppress_price++; /* affects containers */
    ltmp = bp->price * quan;
    buy = PAY_BUY; /* flag; if changed then return early */

    if (itemize) {
        char qbuf[BUFSZ], qsfx[BUFSZ];

/*JP
        Sprintf(qsfx, " for %ld %s.  Pay?", ltmp, currency(ltmp));
*/
        Sprintf(qsfx, "��%ld%s�ł�.�����܂����H", ltmp, currency(ltmp));
#if 0 /*JP*/
        (void) safe_qbuf(qbuf, (char *) 0, qsfx, obj,
                         (quan == 1L) ? Doname2 : doname, ansimpleoname,
                         (quan == 1L) ? "that" : "those");
#else
        (void) safe_qbuf(qbuf, (char *) 0, qsfx, obj,
                         (quan == 1L) ? Doname2 : doname, ansimpleoname,
                         "����");
#endif
        if (yn(qbuf) == 'n') {
            buy = PAY_SKIP;                         /* don't want to buy */
        } else if (quan < bp->bquan && !consumed) { /* partly used goods */
            obj->quan = bp->bquan - save_quan;      /* used up amount */
            if (!Deaf && !muteshk(shkp)) {
#if 0 /*JP*/
                verbalize("%s for the other %s before buying %s.",
                      ANGRY(shkp) ? "Pay" : "Please pay",
                      simpleonames(obj), /* short name suffices */
                      save_quan > 1L ? "these" : "this one");
#else
                verbalize("����𔃂��܂��ɑ���%s��%s",
                      simpleonames(obj),
                      ANGRY(shkp) ? "�����I" : "�����Ă��������D");
#endif
            } else {
#if 0 /*JP*/
                pline("%s %s%s your bill for the other %s first.",
                      Shknam(shkp),
                      ANGRY(shkp) ? "angrily " : "",
                      nolimbs(shkp->data) ? "motions to" : "points out",
                      simpleonames(obj));
#else
                pline("%s��%s����%s�̂��߂̎x�������ɂ���悤�ɑ������D",
                      Shknam(shkp),
                      ANGRY(shkp) ? "�{����" : "",
                      simpleonames(obj));
#endif
            }
            buy = PAY_SKIP; /* shk won't sell */
        }
    }
    if (buy == PAY_BUY && umoney + ESHK(shkp)->credit < ltmp) {
#if 0 /*JP*/
        You("don't%s have gold%s enough to pay for %s.",
            stashed_gold ? " seem to" : "",
            (ESHK(shkp)->credit > 0L) ? " or credit" : "",
            thesimpleoname(obj));
#else
        You("%s�̑�����x���������̂���%s�����ĂȂ�%s�D",
            thesimpleoname(obj),
            (ESHK(shkp)->credit > 0L) ? "���a������" : "��",
            stashed_gold ? "�悤��" : "");
#endif
        buy = itemize ? PAY_SKIP : PAY_CANT;
    }

    if (buy != PAY_BUY) {
        /* restore unpaid object to original state */
        obj->quan = save_quan;
        obj->unpaid = 1;
        iflags.suppress_price--;
        return buy;
    }

    pay(ltmp, shkp);
#if 0 /*JP*/
    shk_names_obj(shkp, obj,
                  consumed ? "paid for %s at a cost of %ld gold piece%s.%s"
                           : "bought %s for %ld gold piece%s.%s",
                  ltmp, "");
#else
    shk_names_obj(shkp, obj,
                  consumed ? "%s�ɋ���%ld��%s���x�������D%s"
                           : "%s������%ld��%s�Ŕ������D%s",
                  ltmp, "");
#endif
    obj->quan = save_quan; /* restore original count */
    /* quan => amount just bought, save_quan => remaining unpaid count */
    if (consumed) {
        if (quan != bp->bquan) {
            /* eliminate used-up portion; remainder is still unpaid */
            bp->bquan = obj->quan;
            obj->unpaid = 1;
            bp->useup = 0;
            buy = PAY_SOME;
        } else { /* completely used-up, so get rid of it */
            obj_extract_self(obj);
            /* assert( obj == *obj_p ); */
            dealloc_obj(obj);
            *obj_p = 0; /* destroy pointer to freed object */
        }
    } else if (itemize)
        update_inventory(); /* Done just once in dopay() if !itemize. */
    iflags.suppress_price--;
    return buy;
}

static struct repo { /* repossession context */
    struct monst *shopkeeper;
    coord location;
} repo;

/* routine called after dying (or quitting) */
boolean
paybill(croaked)
int croaked; /* -1: escaped dungeon; 0: quit; 1: died */
{
    struct monst *mtmp, *mtmp2, *firstshk, *resident, *creditor, *hostile,
        *localshk;
    struct eshk *eshkp;
    boolean taken = FALSE, local;
    int numsk = 0;

    /* if we escaped from the dungeon, shopkeepers can't reach us;
       shops don't occur on level 1, but this could happen if hero
       level teleports out of the dungeon and manages not to die */
    if (croaked < 0)
        return FALSE;
    /* [should probably also return false when dead hero has been
        petrified since shk shouldn't be able to grab inventory
        which has been shut inside a statue] */

    /* this is where inventory will end up if any shk takes it */
    repo.location.x = repo.location.y = 0;
    repo.shopkeeper = 0;

    /*
     * Scan all shopkeepers on the level, to prioritize them:
     * 1) keeper of shop hero is inside and who is owed money,
     * 2) keeper of shop hero is inside who isn't owed any money,
     * 3) other shk who is owed money, 4) other shk who is angry,
     * 5) any shk local to this level, and if none is found,
     * 6) first shk on monster list (last resort; unlikely, since
     * any nonlocal shk will probably be in the owed category
     * and almost certainly be in the angry category).
     */
    resident = creditor = hostile = localshk = (struct monst *) 0;
    for (mtmp = next_shkp(fmon, FALSE); mtmp;
         mtmp = next_shkp(mtmp2, FALSE)) {
        mtmp2 = mtmp->nmon;
        eshkp = ESHK(mtmp);
        local = on_level(&eshkp->shoplevel, &u.uz);
        if (local && index(u.ushops, eshkp->shoproom)) {
            /* inside this shk's shop [there might be more than one
               resident shk if hero is standing in a breech of a shared
               wall, so give priority to one who's also owed money] */
            if (!resident || eshkp->billct || eshkp->debit || eshkp->robbed)
                resident = mtmp;
        } else if (eshkp->billct || eshkp->debit || eshkp->robbed) {
            /* owe this shopkeeper money (might also owe others) */
            if (!creditor)
                creditor = mtmp;
        } else if (eshkp->following || ANGRY(mtmp)) {
            /* this shopkeeper is antagonistic (others might be too) */
            if (!hostile)
                hostile = mtmp;
        } else if (local) {
            /* this shopkeeper's shop is on current level */
            if (!localshk)
                localshk = mtmp;
        }
    }

    /* give highest priority shopkeeper first crack */
    firstshk = resident ? resident
                        : creditor ? creditor
                                   : hostile ? hostile
                                             : localshk;
    if (firstshk) {
        numsk++;
        taken = inherits(firstshk, numsk, croaked);
    }

    /* now handle the rest */
    for (mtmp = next_shkp(fmon, FALSE); mtmp;
         mtmp = next_shkp(mtmp2, FALSE)) {
        mtmp2 = mtmp->nmon;
        eshkp = ESHK(mtmp);
        local = on_level(&eshkp->shoplevel, &u.uz);
        if (mtmp != firstshk) {
            numsk++;
            taken |= inherits(mtmp, numsk, croaked);
        }
        /* for bones: we don't want a shopless shk around */
        if (!local)
            mongone(mtmp);
    }
    return taken;
}

STATIC_OVL boolean
inherits(shkp, numsk, croaked)
struct monst *shkp;
int numsk;
int croaked;
{
    long loss = 0L;
    long umoney;
    struct eshk *eshkp = ESHK(shkp);
    boolean take = FALSE, taken = FALSE;
    unsigned save_minvis = shkp->minvis;
    int roomno = *u.ushops;
    char takes[BUFSZ];

    shkp->minvis = 0;
    /* The simplifying principle is that first-come
       already took everything you had. */
    if (numsk > 1) {
        if (cansee(shkp->mx, shkp->my) && croaked) {
            takes[0] = '\0';
            if (has_head(shkp->data) && !rn2(2))
#if 0 /*JP*/
                Sprintf(takes, ", shakes %s %s,", mhis(shkp),
                        mbodypart(shkp, HEAD));
#else
                Strcpy(takes, "���U��C");
#endif
#if 0 /*JP*/
            pline("%s %slooks at your corpse%s and %s.", Shknam(shkp),
                  (!shkp->mcanmove || shkp->msleeping) ? "wakes up, " : "",
                  takes, !inhishop(shkp) ? "disappears" : "sighs");
#else
            pline("%s��%s���Ȃ��̎��̂�����%s%s�D", Shknam(shkp),
                  (!shkp->mcanmove || shkp->msleeping) ? "�ڂ����܂���" : "",
                  takes, !inhishop(shkp) ? "�p��������" : "����������");
#endif
        }
        rouse_shk(shkp, FALSE); /* wake shk for bones */
        taken = (roomno == eshkp->shoproom);
        goto skip;
    }

    /* get one case out of the way: you die in the shop, the */
    /* shopkeeper is peaceful, nothing stolen, nothing owed. */
    if (roomno == eshkp->shoproom && inhishop(shkp) && !eshkp->billct
        && !eshkp->robbed && !eshkp->debit && NOTANGRY(shkp)
        && !eshkp->following && u.ugrave_arise < LOW_PM) {
        taken = (invent != 0);
        if (taken)
/*JP
            pline("%s gratefully inherits all your possessions.",
*/
            pline("%s�͂��Ȃ��̎����������肪�����󂯂Ƃ����D",
                  shkname(shkp));
        set_repo_loc(shkp);
        goto clear;
    }

    if (eshkp->billct || eshkp->debit || eshkp->robbed) {
        if (roomno == eshkp->shoproom && inhishop(shkp))
            loss = addupbill(shkp) + eshkp->debit;
        if (loss < eshkp->robbed)
            loss = eshkp->robbed;
        take = TRUE;
    }

    if (eshkp->following || ANGRY(shkp) || take) {
        if (!invent)
            goto skip;
        umoney = money_cnt(invent);
        takes[0] = '\0';
        if (!shkp->mcanmove || shkp->msleeping)
/*JP
            Strcat(takes, "wakes up and ");
*/
            Strcat(takes, "�ڂ����߂�ƁC");
        if (distu(shkp->mx, shkp->my) > 2)
/*JP
            Strcat(takes, "comes and ");
*/
            Strcat(takes, "�߂Â��C");
#if 0 /*JP*/
        Strcat(takes, "takes");
#endif

        if (loss > umoney || !loss || roomno == eshkp->shoproom) {
            eshkp->robbed -= umoney;
            if (eshkp->robbed < 0L)
                eshkp->robbed = 0L;
            if (umoney > 0)
                money2mon(shkp, umoney);
            context.botl = 1;
/*JP
            pline("%s %s all your possessions.", shkname(shkp), takes);
*/
            pline("%s��%s���Ȃ��̎��������ׂĂ���������D", shkname(shkp), takes);
            taken = TRUE;
            /* where to put player's invent (after disclosure) */
            set_repo_loc(shkp);
        } else {
            money2mon(shkp, loss);
            context.botl = 1;
#if 0 /*JP*/
            pline("%s %s the %ld %s %sowed %s.", Shknam(shkp),
                  takes, loss, currency(loss),
                  strncmp(eshkp->customer, plname, PL_NSIZ) ? "" : "you ",
                  mhim(shkp));
#else
            pline("%s��%s%s�؂�Ă���%ld%s���󂯂Ƃ����D", Shknam(shkp),
                  takes,
                  strncmp(eshkp->customer, plname, PL_NSIZ) ? "" : "���Ȃ���",
                  loss, currency(loss));
#endif
            /* shopkeeper has now been paid in full */
            pacify_shk(shkp);
            eshkp->following = 0;
            eshkp->robbed = 0L;
        }
    skip:
        /* in case we create bones */
        rouse_shk(shkp, FALSE); /* wake up */
        if (!inhishop(shkp))
            home_shk(shkp, FALSE);
    }
clear:
    shkp->minvis = save_minvis;
    setpaid(shkp);
    return taken;
}

STATIC_OVL void
set_repo_loc(shkp)
struct monst *shkp;
{
    register xchar ox, oy;
    struct eshk *eshkp = ESHK(shkp);

    /* if you're not in this shk's shop room, or if you're in its doorway
        or entry spot, then your gear gets dumped all the way inside */
    if (*u.ushops != eshkp->shoproom || IS_DOOR(levl[u.ux][u.uy].typ)
        || (u.ux == eshkp->shk.x && u.uy == eshkp->shk.y)) {
        /* shk.x,shk.y is the position immediately in
         * front of the door -- move in one more space
         */
        ox = eshkp->shk.x;
        oy = eshkp->shk.y;
        ox += sgn(ox - eshkp->shd.x);
        oy += sgn(oy - eshkp->shd.y);
    } else { /* already inside this shk's shop */
        ox = u.ux;
        oy = u.uy;
    }
    /* finish_paybill will deposit invent here */
    repo.location.x = ox;
    repo.location.y = oy;
    repo.shopkeeper = shkp;
}

/* called at game exit, after inventory disclosure but before making bones */
void
finish_paybill()
{
    struct monst *shkp = repo.shopkeeper;
    int ox = repo.location.x, oy = repo.location.y;

#if 0 /* don't bother */
    if (ox == 0 && oy == 0)
        impossible("finish_paybill: no location");
#endif
    /* normally done by savebones(), but that's too late in this case */
    unleash_all();
    /* if hero has any gold left, take it into shopkeeper's possession */
    if (shkp) {
        long umoney = money_cnt(invent);

        if (umoney)
            money2mon(shkp, umoney);
    }
    /* transfer rest of the character's inventory to the shop floor */
    drop_upon_death((struct monst *) 0, (struct obj *) 0, ox, oy);
}

/* find obj on one of the lists */
STATIC_OVL struct obj *
bp_to_obj(bp)
register struct bill_x *bp;
{
    register struct obj *obj;
    register unsigned int id = bp->bo_id;

    if (bp->useup)
        obj = o_on(id, billobjs);
    else
        obj = find_oid(id);
    return obj;
}

/*
 * Look for o_id on all lists but billobj.  Return obj or NULL if not found.
 * Its OK for restore_timers() to call this function, there should not
 * be any timeouts on the billobjs chain.
 */
struct obj *
find_oid(id)
unsigned id;
{
    struct obj *obj;
    struct monst *mon, *mmtmp[3];
    int i;

    /* first check various obj lists directly */
    if ((obj = o_on(id, invent)) != 0)
        return obj;
    if ((obj = o_on(id, fobj)) != 0)
        return obj;
    if ((obj = o_on(id, level.buriedobjlist)) != 0)
        return obj;
    if ((obj = o_on(id, migrating_objs)) != 0)
        return obj;

    /* not found yet; check inventory for members of various monst lists */
    mmtmp[0] = fmon;
    mmtmp[1] = migrating_mons;
    mmtmp[2] = mydogs; /* for use during level changes */
    for (i = 0; i < 3; i++)
        for (mon = mmtmp[i]; mon; mon = mon->nmon)
            if ((obj = o_on(id, mon->minvent)) != 0)
                return obj;

    /* not found at all */
    return (struct obj *) 0;
}

/* Returns the price of an arbitrary item in the shop.
   Returns 0 if the item doesn't belong to a shopkeeper. */
long
get_cost_of_shop_item(obj)
register struct obj *obj;
{
    struct monst *shkp;
    xchar x, y;
    long cost = 0L;

    if (*u.ushops
        && obj->oclass != COIN_CLASS
        && obj != uball && obj != uchain
        && get_obj_location(obj, &x, &y, 0)
        && (obj->unpaid
            || (obj->where == OBJ_FLOOR
                && !obj->no_charge && costly_spot(x, y)))
        && (shkp = shop_keeper(*in_rooms(x, y, SHOPBASE))) != 0
        && inhishop(shkp)) {
        cost = obj->quan * get_cost(obj, shkp);
        if (Has_contents(obj))
            cost += contained_cost(obj, shkp, 0L, FALSE, FALSE);
    }
    return cost;
}

/* calculate the value that the shk will charge for [one of] an object */
STATIC_OVL long
get_cost(obj, shkp)
register struct obj *obj;
register struct monst *shkp; /* if angry, impose a surcharge */
{
    long tmp = getprice(obj, FALSE),
         /* used to perform a single calculation even when multiple
            adjustments (unID'd, dunce/tourist, charisma) are made */
        multiplier = 1L, divisor = 1L;

    if (!tmp)
        tmp = 5L;
    /* shopkeeper may notice if the player isn't very knowledgeable -
       especially when gem prices are concerned */
    if (!obj->dknown || !objects[obj->otyp].oc_name_known) {
        if (obj->oclass == GEM_CLASS
            && objects[obj->otyp].oc_material == GLASS) {
            int i;
            /* get a value that's 'random' from game to game, but the
               same within the same game */
            boolean pseudorand =
                (((int) ubirthday % obj->otyp) >= obj->otyp / 2);

            /* all gems are priced high - real or not */
            switch (obj->otyp - LAST_GEM) {
            case 1: /* white */
                i = pseudorand ? DIAMOND : OPAL;
                break;
            case 2: /* blue */
                i = pseudorand ? SAPPHIRE : AQUAMARINE;
                break;
            case 3: /* red */
                i = pseudorand ? RUBY : JASPER;
                break;
            case 4: /* yellowish brown */
                i = pseudorand ? AMBER : TOPAZ;
                break;
            case 5: /* orange */
                i = pseudorand ? JACINTH : AGATE;
                break;
            case 6: /* yellow */
                i = pseudorand ? CITRINE : CHRYSOBERYL;
                break;
            case 7: /* black */
                i = pseudorand ? BLACK_OPAL : JET;
                break;
            case 8: /* green */
                i = pseudorand ? EMERALD : JADE;
                break;
            case 9: /* violet */
                i = pseudorand ? AMETHYST : FLUORITE;
                break;
            default:
                impossible("bad glass gem %d?", obj->otyp);
                i = STRANGE_OBJECT;
                break;
            }
            tmp = (long) objects[i].oc_cost;
        } else if (!(obj->o_id % 4)) {
            /* unid'd, arbitrarily impose surcharge: tmp *= 4/3 */
            multiplier *= 4L;
            divisor *= 3L;
        }
    }
    if (uarmh && uarmh->otyp == DUNCE_CAP)
        multiplier *= 4L, divisor *= 3L;
    else if ((Role_if(PM_TOURIST) && u.ulevel < (MAXULEV / 2))
             || (uarmu && !uarm && !uarmc)) /* touristy shirt visible */
        multiplier *= 4L, divisor *= 3L;

    if (ACURR(A_CHA) > 18)
        divisor *= 2L;
    else if (ACURR(A_CHA) == 18)
        multiplier *= 2L, divisor *= 3L;
    else if (ACURR(A_CHA) >= 16)
        multiplier *= 3L, divisor *= 4L;
    else if (ACURR(A_CHA) <= 5)
        multiplier *= 2L;
    else if (ACURR(A_CHA) <= 7)
        multiplier *= 3L, divisor *= 2L;
    else if (ACURR(A_CHA) <= 10)
        multiplier *= 4L, divisor *= 3L;

    /* tmp = (tmp * multiplier) / divisor [with roundoff tweak] */
    tmp *= multiplier;
    if (divisor > 1L) {
        /* tmp = (((tmp * 10) / divisor) + 5) / 10 */
        tmp *= 10L;
        tmp /= divisor;
        tmp += 5L;
        tmp /= 10L;
    }

    if (tmp <= 0L)
        tmp = 1L;
    /* the artifact prices in artilist[] are also used as a score bonus;
       inflate their shop price here without affecting score calculation */
    if (obj->oartifact)
        tmp *= 4L;

    /* anger surcharge should match rile_shk's, so we do it separately
       from the multiplier/divisor calculation */
    if (shkp && ESHK(shkp)->surcharge)
        tmp += (tmp + 2L) / 3L;
    return tmp;
}

/* returns the price of a container's content.  the price
 * of the "top" container is added in the calling functions.
 * a different price quoted for selling as vs. buying.
 */
long
contained_cost(obj, shkp, price, usell, unpaid_only)
struct obj *obj;
struct monst *shkp;
long price;
boolean usell;
boolean unpaid_only;
{
    register struct obj *otmp;

    /* price of contained objects; "top" container handled by caller */
    for (otmp = obj->cobj; otmp; otmp = otmp->nobj) {
        if (otmp->oclass == COIN_CLASS)
            continue;

        if (usell) {
            if (saleable(shkp, otmp) && !otmp->unpaid
                && otmp->oclass != BALL_CLASS
                && !(otmp->oclass == FOOD_CLASS && otmp->oeaten)
                && !(Is_candle(otmp)
                     && otmp->age < 20L * (long) objects[otmp->otyp].oc_cost))
                price += set_cost(otmp, shkp);
        } else if (!otmp->no_charge && (otmp->unpaid || !unpaid_only)) {
            price += get_cost(otmp, shkp) * otmp->quan;
        }

        if (Has_contents(otmp))
            price = contained_cost(otmp, shkp, price, usell, unpaid_only);
    }

    return price;
}

/* count amount of gold inside container 'obj' and any nested containers */
long
contained_gold(obj)
struct obj *obj;
{
    register struct obj *otmp;
    register long value = 0L;

    /* accumulate contained gold */
    for (otmp = obj->cobj; otmp; otmp = otmp->nobj)
        if (otmp->oclass == COIN_CLASS)
            value += otmp->quan;
        else if (Has_contents(otmp))
            value += contained_gold(otmp);

    return value;
}

STATIC_OVL void
dropped_container(obj, shkp, sale)
register struct obj *obj;
register struct monst *shkp;
register boolean sale;
{
    register struct obj *otmp;

    /* the "top" container is treated in the calling fn */
    for (otmp = obj->cobj; otmp; otmp = otmp->nobj) {
        if (otmp->oclass == COIN_CLASS)
            continue;

        if (!otmp->unpaid && !(sale && saleable(shkp, otmp)))
            otmp->no_charge = 1;

        if (Has_contents(otmp))
            dropped_container(otmp, shkp, sale);
    }
}

void
picked_container(obj)
register struct obj *obj;
{
    register struct obj *otmp;

    /* the "top" container is treated in the calling fn */
    for (otmp = obj->cobj; otmp; otmp = otmp->nobj) {
        if (otmp->oclass == COIN_CLASS)
            continue;

        if (otmp->no_charge)
            otmp->no_charge = 0;

        if (Has_contents(otmp))
            picked_container(otmp);
    }
}

STATIC_OVL boolean
special_stock(obj, shkp, quietly)
struct obj *obj;
struct monst *shkp;
boolean quietly;
{
    /* for unique situations */
    if (ESHK(shkp)->shoptype == CANDLESHOP
        && obj->otyp == CANDELABRUM_OF_INVOCATION) {
        if (!quietly) {
            if (is_izchak(shkp, TRUE) && !u.uevent.invoked) {
                if (Deaf || muteshk(shkp)) {
#if 0 /*JP:T*/
                    pline("%s seems %s that you want to sell that.",
                          Shknam(shkp),
                          (obj->spe < 7) ? "horrified" : "concerned");
#else
                    pline("%s�͂��Ȃ�������𔄂낤�Ƃ��Ă��邱�Ƃ�%s�Ă���悤���D",
                          Shknam(shkp),
                          (obj->spe < 7) ? "����" : "�S�z��");
#endif
                } else {
/*JP
                verbalize("No thanks, I'd hang onto that if I were you.");
*/
                verbalize("����C����Ȃ��D���Ȃ炻���������Ȃ��ˁD");
                    if (obj->spe < 7)
#if 0 /*JP:T*/
                        verbalize(
                             "You'll need %d%s candle%s to go along with it.",
                                (7 - obj->spe), (obj->spe > 0) ? " more" : "",
                                  plur(7 - obj->spe));
#else
                        verbalize(
                             "������g���ɂ�%s%d�{�̂낤�������K�v���D",
                              (obj->spe > 0) ? "����" : "", (7 - obj->spe));
#endif
                    /* [what if hero is already carrying enough candles?
                       should Izchak explain how to attach them instead?] */
                }
            } else {
                if (!Deaf && !muteshk(shkp))
/*JP
                verbalize("I won't stock that.  Take it out of here!");
*/
                verbalize("����͎d����Ȃ���D�����Ă����Ȃ����I");
                else
#if 0 /*JP*/
                    pline("%s shakes %s %s in refusal.",
                            Shknam(shkp),
                            mhis(shkp),
                            mbodypart(shkp, HEAD));
#else
                    pline("%s��%s��U���ċ��ۂ����D",
                            Shknam(shkp),
                            mbodypart(shkp, HEAD));
#endif
            }
        }
        return TRUE;
    }
    return FALSE;
}

/* calculate how much the shk will pay when buying [all of] an object */
STATIC_OVL long
set_cost(obj, shkp)
register struct obj *obj;
register struct monst *shkp;
{
    long tmp = getprice(obj, TRUE) * obj->quan, multiplier = 1L, divisor = 1L;

    if (uarmh && uarmh->otyp == DUNCE_CAP)
        divisor *= 3L;
    else if ((Role_if(PM_TOURIST) && u.ulevel < (MAXULEV / 2))
             || (uarmu && !uarm && !uarmc)) /* touristy shirt visible */
        divisor *= 3L;
    else
        divisor *= 2L;

    /* shopkeeper may notice if the player isn't very knowledgeable -
       especially when gem prices are concerned */
    if (!obj->dknown || !objects[obj->otyp].oc_name_known) {
        if (obj->oclass == GEM_CLASS) {
            /* different shop keepers give different prices */
            if (objects[obj->otyp].oc_material == GEMSTONE
                || objects[obj->otyp].oc_material == GLASS) {
                tmp = (obj->otyp % (6 - shkp->m_id % 3));
                tmp = (tmp + 3) * obj->quan;
            }
        } else if (tmp > 1L && !(shkp->m_id % 4))
            multiplier *= 3L, divisor *= 4L;
    }

    if (tmp >= 1L) {
        /* [see get_cost()] */
        tmp *= multiplier;
        if (divisor > 1L) {
            tmp *= 10L;
            tmp /= divisor;
            tmp += 5L;
            tmp /= 10L;
        }
        /* avoid adjusting nonzero to zero */
        if (tmp < 1L)
            tmp = 1L;
    }

    /* (no adjustment for angry shk here) */
    return tmp;
}

/* called when an item's value has been enhanced; if it happens to be
   on any shop bill, update that bill to reflect the new higher price
   [if the new price drops for some reason, keep the old one in place] */
void
alter_cost(obj, amt)
struct obj *obj;
long amt; /* if 0, use regular shop pricing, otherwise force amount;
             if negative, use abs(amt) even if it's less than old cost */
{
    struct bill_x *bp = 0;
    struct monst *shkp;
    long new_price;

    for (shkp = next_shkp(fmon, TRUE); shkp; shkp = next_shkp(shkp, TRUE))
        if ((bp = onbill(obj, shkp, TRUE)) != 0) {
            new_price = !amt ? get_cost(obj, shkp) : (amt < 0L) ? -amt : amt;
            if (new_price > bp->price || amt < 0L) {
                bp->price = new_price;
                update_inventory();
            }
            break; /* done */
        }
    return;
}

/* called from doinv(invent.c) for inventory of unpaid objects */
long
unpaid_cost(unp_obj, include_contents)
struct obj *unp_obj; /* known to be unpaid or contain unpaid */
boolean include_contents;
{
    struct bill_x *bp = (struct bill_x *) 0;
    struct monst *shkp;
    long amt = 0L;
    xchar ox, oy;

    if (!get_obj_location(unp_obj, &ox, &oy, BURIED_TOO | CONTAINED_TOO))
        ox = u.ux, oy = u.uy; /* (shouldn't happen) */
    if ((shkp = shop_keeper(*in_rooms(ox, oy, SHOPBASE))) != 0) {
        bp = onbill(unp_obj, shkp, TRUE);
    } else {
        /* didn't find shk?  try searching bills */
        for (shkp = next_shkp(fmon, TRUE); shkp;
             shkp = next_shkp(shkp->nmon, TRUE))
            if ((bp = onbill(unp_obj, shkp, TRUE)) != 0)
                break;
    }

    /* onbill() gave no message if unexpected problem occurred */
    if (!shkp || (unp_obj->unpaid && !bp)) {
        impossible("unpaid_cost: object wasn't on any bill.");
    } else {
        if (bp)
            amt = unp_obj->quan * bp->price;
        if (include_contents && Has_contents(unp_obj))
            amt = contained_cost(unp_obj, shkp, amt, FALSE, TRUE);
    }
    return amt;
}

STATIC_OVL void
add_one_tobill(obj, dummy, shkp)
struct obj *obj;
boolean dummy;
struct monst *shkp;
{
    struct eshk *eshkp;
    struct bill_x *bp;
    int bct;

    if (!billable(&shkp, obj, *u.ushops, TRUE))
        return;
    eshkp = ESHK(shkp);

    if (eshkp->billct == BILLSZ) {
/*JP
        You("got that for free!");
*/
        You("����������Ŏ�ɓ��ꂽ�I");
        return;
    }

    /* normally bill_p gets set up whenever you enter the shop, but obj
       might be going onto the bill because hero just snagged it with
       a grappling hook from outside without ever having been inside */
    if (!eshkp->bill_p)
        eshkp->bill_p = &(eshkp->bill[0]);

    bct = eshkp->billct;
    bp = &(eshkp->bill_p[bct]);
    bp->bo_id = obj->o_id;
    bp->bquan = obj->quan;
    if (dummy) {              /* a dummy object must be inserted into  */
        bp->useup = 1;        /* the billobjs chain here.  crucial for */
        add_to_billobjs(obj); /* eating floorfood in shop.  see eat.c  */
    } else
        bp->useup = 0;
    bp->price = get_cost(obj, shkp);
    eshkp->billct++;
    obj->unpaid = 1;
}

STATIC_OVL void
add_to_billobjs(obj)
struct obj *obj;
{
    if (obj->where != OBJ_FREE)
        panic("add_to_billobjs: obj not free");
    if (obj->timed)
        obj_stop_timers(obj);

    obj->nobj = billobjs;
    billobjs = obj;
    obj->where = OBJ_ONBILL;
}

/* recursive billing of objects within containers. */
STATIC_OVL void
bill_box_content(obj, ininv, dummy, shkp)
register struct obj *obj;
register boolean ininv, dummy;
register struct monst *shkp;
{
    register struct obj *otmp;

    for (otmp = obj->cobj; otmp; otmp = otmp->nobj) {
        if (otmp->oclass == COIN_CLASS)
            continue;

        /* the "top" box is added in addtobill() */
        if (!otmp->no_charge)
            add_one_tobill(otmp, dummy, shkp);
        if (Has_contents(otmp))
            bill_box_content(otmp, ininv, dummy, shkp);
    }
}

/* shopkeeper tells you what you bought or sold, sometimes partly IDing it */
STATIC_OVL void
shk_names_obj(shkp, obj, fmt, amt, arg)
struct monst *shkp;
struct obj *obj;
const char *fmt; /* "%s %ld %s %s", doname(obj), amt, plur(amt), arg */
long amt;
const char *arg;
{
#if 0 /*JP*/
    char *obj_name, fmtbuf[BUFSZ];
#else
    char *obj_name;
#endif
    boolean was_unknown = !obj->dknown;

    obj->dknown = TRUE;
    /* Use real name for ordinary weapons/armor, and spell-less
     * scrolls/books (that is, blank and mail), but only if the
     * object is within the shk's area of interest/expertise.
     */
    if (!objects[obj->otyp].oc_magic && saleable(shkp, obj)
        && (obj->oclass == WEAPON_CLASS || obj->oclass == ARMOR_CLASS
            || obj->oclass == SCROLL_CLASS || obj->oclass == SPBOOK_CLASS
            || obj->otyp == MIRROR)) {
        was_unknown |= !objects[obj->otyp].oc_name_known;
        makeknown(obj->otyp);
    }
    obj_name = doname(obj);
    /* Use an alternate message when extra information is being provided */
#if 0 /*JP*//*���{��ł͓�������*/
    if (was_unknown) {
        Sprintf(fmtbuf, "%%s; you %s", fmt);
        obj_name[0] = highc(obj_name[0]);
        pline(fmtbuf, obj_name, (obj->quan > 1L) ? "them" : "it", amt,
              plur(amt), arg);
    } else {
        You(fmt, obj_name, amt, plur(amt), arg);
    }
#else
    You(fmt, obj_name, amt, "", arg);
#endif
}

/* decide whether a shopkeeper thinks an item belongs to her */
boolean
billable(shkpp, obj, roomno, reset_nocharge)
struct monst **shkpp; /* in: non-null if shk has been validated; out: shk */
struct obj *obj;
char roomno;
boolean reset_nocharge;
{
    struct monst *shkp = *shkpp;

    /* if caller hasn't supplied a shopkeeper, look one up now */
    if (!shkp) {
        if (!roomno)
            return FALSE;
        shkp = shop_keeper(roomno);
        if (!shkp || !inhishop(shkp))
            return FALSE;
        *shkpp = shkp;
    }
    /* perhaps we threw it away earlier */
    if (onbill(obj, shkp, FALSE)
        || (obj->oclass == FOOD_CLASS && obj->oeaten))
        return FALSE;
    /* outer container might be marked no_charge but still have contents
       which should be charged for; clear no_charge when picking things up */
    if (obj->no_charge) {
        if (!Has_contents(obj) || (contained_gold(obj) == 0L
                                   && contained_cost(obj, shkp, 0L, FALSE,
                                                     !reset_nocharge) == 0L))
            shkp = 0; /* not billable */
        if (reset_nocharge && !shkp && obj->oclass != COIN_CLASS) {
            obj->no_charge = 0;
            if (Has_contents(obj))
                picked_container(obj); /* clear no_charge */
        }
    }
    return shkp ? TRUE : FALSE;
}

void
addtobill(obj, ininv, dummy, silent)
struct obj *obj;
boolean ininv, dummy, silent;
{
    struct monst *shkp = 0;
    long ltmp, cltmp, gltmp;
    int contentscount;
    boolean container;

    if (!billable(&shkp, obj, *u.ushops, TRUE))
        return;

    if (obj->oclass == COIN_CLASS) {
        costly_gold(obj->ox, obj->oy, obj->quan);
        return;
    } else if (ESHK(shkp)->billct == BILLSZ) {
        if (!silent)
/*JP
            You("got that for free!");
*/
            You("����������Ŏ�ɓ��ꂽ�I");
        return;
    }

    ltmp = cltmp = gltmp = 0L;
    container = Has_contents(obj);

    if (!obj->no_charge)
        ltmp = get_cost(obj, shkp);

    if (obj->no_charge && !container) {
        obj->no_charge = 0;
        return;
    }

    if (container) {
        cltmp = contained_cost(obj, shkp, cltmp, FALSE, FALSE);
        gltmp = contained_gold(obj);

        if (ltmp)
            add_one_tobill(obj, dummy, shkp);
        if (cltmp)
            bill_box_content(obj, ininv, dummy, shkp);
        picked_container(obj); /* reset contained obj->no_charge */

        ltmp += cltmp;

        if (gltmp) {
            costly_gold(obj->ox, obj->oy, gltmp);
            if (!ltmp)
                return;
        }

        if (obj->no_charge)
            obj->no_charge = 0;
        contentscount = count_unpaid(obj->cobj);
    } else { /* !container */
        add_one_tobill(obj, dummy, shkp);
        contentscount = 0;
    }

    if (!Deaf && !muteshk(shkp) && !silent) {
        char buf[BUFSZ];

        if (!ltmp) {
/*JP
            pline("%s has no interest in %s.", Shknam(shkp), the(xname(obj)));
*/
            pline("%s��%s�ɋ����������Ȃ��D", Shknam(shkp), the(xname(obj)));
            return;
        }
        if (!ininv) {
#if 0 /*JP*/
            pline("%s will cost you %ld %s%s.", The(xname(obj)), ltmp,
                  currency(ltmp), (obj->quan > 1L) ? " each" : "");
#else
            pline("%s��%s��������%ld%s���D", xname(obj),
                  (obj->quan > 1L) ? "���" : "", ltmp, currency(ltmp));
#endif
        } else {
            long save_quan = obj->quan;

#if 0 /*JP*/
            Strcpy(buf, "\"For you, ");
            if (ANGRY(shkp)) {
                Strcat(buf, "scum;");
            } else {
                append_honorific(buf);
                Strcat(buf, "; only");
            }
#else
            Strcpy(buf, "�u");
            if (ANGRY(shkp)) {
                Strcat(buf, "���̃N�\������C");
            } else {
                append_honorific(buf);
                Strcat(buf, "�C");
            }
#endif
            obj->quan = 1L; /* fool xname() into giving singular */
#if 0 /*JP*/
            pline("%s %ld %s %s %s%s.\"", buf, ltmp, currency(ltmp),
                  (save_quan > 1L) ? "per"
                                   : (contentscount && !obj->unpaid)
                                       ? "for the contents of this"
                                       : "for this",
                  xname(obj),
                  (contentscount && obj->unpaid) ? and_its_contents : "");
#else
            pline("%s%s%s��������%ld%s���D�v", buf, xname(obj),
                  (save_quan > 1L) ? "�͈��" : (contentscount && !obj->unpaid)
                                                 ? "�̒��g��"
                                                 : "��",
                  ltmp, currency(ltmp));
#endif
            obj->quan = save_quan;
        }
    } else if (!silent) {
        if (ltmp)
#if 0 /*JP*/
            pline_The("list price of %s%s%s is %ld %s%s.",
                      (contentscount && !obj->unpaid) ? the_contents_of : "",
                      the(xname(obj)),
                      (contentscount && obj->unpaid) ? and_its_contents : "",
                      ltmp, currency(ltmp), (obj->quan > 1L) ? " each" : "");
#else
            pline_The("%s%s%s%s��������%ld%s��.",
                      the(xname(obj)),
                      (contentscount && !obj->unpaid) ? "�̒��g��" : "",
                      (contentscount && obj->unpaid) ? and_its_contents : "",
                      (obj->quan > 1L) ? "���ꂼ��" : "", ltmp, currency(ltmp));
#endif
        else
/*JP
            pline("%s does not notice.", Shknam(shkp));
*/
            pline("%s�͋C�����Ă��Ȃ��D", Shknam(shkp));
    }
}

void
append_honorific(buf)
char *buf;
{
    /* (chooses among [0]..[3] normally; [1]..[4] after the
       Wizard has been killed or invocation ritual performed) */
#if 0 /*JP*/
    static const char *const honored[] = { "good", "honored", "most gracious",
                                           "esteemed",
                                           "most renowned and sacred" };
#else
    static const char *const honored[] = {
        "�₟", "�₟�C���_����", "�₟�C��i��",
        "�₟�C���h����",
        "�₟�C�����Ő_����"
        };
#endif
    Strcat(buf, honored[rn2(SIZE(honored) - 1) + u.uevent.udemigod]);
#if 0 /*JP*//*���{��ł͂����܂ł��Ȃ�*/
    if (is_vampire(youmonst.data))
        Strcat(buf, (flags.female) ? " dark lady" : " dark lord");
    else if (is_elf(youmonst.data))
        Strcat(buf, (flags.female) ? " hiril" : " hir");
    else
#endif
#if 0 /*JP*/
        Strcat(buf, !is_human(youmonst.data) ? " creature" : (flags.female)
                                                                 ? " lady"
                                                                 : " sir");
#else
        Strcat(buf, !is_human(youmonst.data) ? "��������" : (flags.female)
                                                          ? "���삳��"
                                                          : "�U�߂���");
#endif
}

void
splitbill(obj, otmp)
register struct obj *obj, *otmp;
{
    /* otmp has been split off from obj */
    register struct bill_x *bp;
    register long tmp;
    register struct monst *shkp = shop_keeper(*u.ushops);

    if (!shkp || !inhishop(shkp)) {
        impossible("splitbill: no resident shopkeeper??");
        return;
    }
    bp = onbill(obj, shkp, FALSE);
    if (!bp) {
        impossible("splitbill: not on bill?");
        return;
    }
    if (bp->bquan < otmp->quan) {
        impossible("Negative quantity on bill??");
    }
    if (bp->bquan == otmp->quan) {
        impossible("Zero quantity on bill??");
    }
    bp->bquan -= otmp->quan;

    if (ESHK(shkp)->billct == BILLSZ)
        otmp->unpaid = 0;
    else {
        tmp = bp->price;
        bp = &(ESHK(shkp)->bill_p[ESHK(shkp)->billct]);
        bp->bo_id = otmp->o_id;
        bp->bquan = otmp->quan;
        bp->useup = 0;
        bp->price = tmp;
        ESHK(shkp)->billct++;
    }
}

STATIC_OVL void
sub_one_frombill(obj, shkp)
register struct obj *obj;
register struct monst *shkp;
{
    register struct bill_x *bp;

    if ((bp = onbill(obj, shkp, FALSE)) != 0) {
        register struct obj *otmp;

        obj->unpaid = 0;
        if (bp->bquan > obj->quan) {
            otmp = newobj();
            *otmp = *obj;
            otmp->oextra = (struct oextra *) 0;
            bp->bo_id = otmp->o_id = context.ident++;
            otmp->where = OBJ_FREE;
            otmp->quan = (bp->bquan -= obj->quan);
            otmp->owt = 0; /* superfluous */
            bp->useup = 1;
            add_to_billobjs(otmp);
            return;
        }
        ESHK(shkp)->billct--;
#ifdef DUMB
        {
            /* DRS/NS 2.2.6 messes up -- Peter Kendell */
            int indx = ESHK(shkp)->billct;

            *bp = ESHK(shkp)->bill_p[indx];
        }
#else
        *bp = ESHK(shkp)->bill_p[ESHK(shkp)->billct];
#endif
        return;
    } else if (obj->unpaid) {
        impossible("sub_one_frombill: unpaid object not on bill");
        obj->unpaid = 0;
    }
}

/* recursive check of unpaid objects within nested containers. */
void
subfrombill(obj, shkp)
register struct obj *obj;
register struct monst *shkp;
{
    register struct obj *otmp;

    sub_one_frombill(obj, shkp);

    if (Has_contents(obj))
        for (otmp = obj->cobj; otmp; otmp = otmp->nobj) {
            if (otmp->oclass == COIN_CLASS)
                continue;

            if (Has_contents(otmp))
                subfrombill(otmp, shkp);
            else
                sub_one_frombill(otmp, shkp);
        }
}

STATIC_OVL long
stolen_container(obj, shkp, price, ininv)
struct obj *obj;
struct monst *shkp;
long price;
boolean ininv;
{
    struct obj *otmp;
    struct bill_x *bp;
    long billamt;

    /* the price of contained objects; caller handles top container */
    for (otmp = obj->cobj; otmp; otmp = otmp->nobj) {
        if (otmp->oclass == COIN_CLASS)
            continue;
        billamt = 0L;
        if (!billable(&shkp, otmp, ESHK(shkp)->shoproom, TRUE)) {
            /* billable() returns false for objects already on bill */
            if ((bp = onbill(otmp, shkp, FALSE)) == 0)
                continue;
            /* this assumes that we're being called by stolen_value()
               (or by a recursive call to self on behalf of it) where
               the cost of this object is about to be added to shop
               debt in place of having it remain on the current bill */
            billamt = bp->bquan * bp->price;
            sub_one_frombill(otmp, shkp); /* avoid double billing */
        }

        if (billamt)
            price += billamt;
        else if (ininv ? otmp->unpaid : !otmp->no_charge)
            price += otmp->quan * get_cost(otmp, shkp);

        if (Has_contents(otmp))
            price = stolen_container(otmp, shkp, price, ininv);
    }

    return price;
}

long
stolen_value(obj, x, y, peaceful, silent)
struct obj *obj;
xchar x, y;
boolean peaceful, silent;
{
    long value = 0L, gvalue = 0L, billamt = 0L;
    char roomno = *in_rooms(x, y, SHOPBASE);
    struct bill_x *bp;
    struct monst *shkp = 0;

    if (!billable(&shkp, obj, roomno, FALSE)) {
        /* things already on the bill yield a not-billable result, so
           we need to check bill before deciding that shk doesn't care */
        if ((bp = onbill(obj, shkp, FALSE)) == 0)
            return 0L;
        /* shk does care; take obj off bill to avoid double billing */
        billamt = bp->bquan * bp->price;
        sub_one_frombill(obj, shkp);
    }

    if (obj->oclass == COIN_CLASS) {
        gvalue += obj->quan;
    } else {
        if (billamt)
            value += billamt;
        else if (!obj->no_charge)
            value += obj->quan * get_cost(obj, shkp);

        if (Has_contents(obj)) {
            boolean ininv =
                (obj->where == OBJ_INVENT || obj->where == OBJ_FREE);

            value += stolen_container(obj, shkp, 0L, ininv);
            if (!ininv)
                gvalue += contained_gold(obj);
        }
    }

    if (gvalue + value == 0L)
        return 0L;

    value += gvalue;

    if (peaceful) {
        boolean credit_use = !!ESHK(shkp)->credit;
        value = check_credit(value, shkp);
        /* 'peaceful' affects general treatment, but doesn't affect
         * the fact that other code expects that all charges after the
         * shopkeeper is angry are included in robbed, not debit */
        if (ANGRY(shkp))
            ESHK(shkp)->robbed += value;
        else
            ESHK(shkp)->debit += value;

        if (!silent) {
            const char *still = "";

            if (credit_use) {
                if (ESHK(shkp)->credit) {
#if 0 /*JP*/
                    You("have %ld %s credit remaining.", ESHK(shkp)->credit,
                        currency(ESHK(shkp)->credit));
#else
                    You("%ld%s�̗a�������c���Ă���D", ESHK(shkp)->credit,
                        currency(ESHK(shkp)->credit));
#endif
                    return value;
                } else if (!value) {
/*JP
                    You("have no credit remaining.");
*/
                    pline("�a�����͂Ȃ��Ȃ����D");
                    return 0;
                }
/*JP
                still = "still ";
*/
                still = "�����";
            }
            if (obj->oclass == COIN_CLASS)
#if 0 /*JP*/
                You("%sowe %s %ld %s!", still, shkname(shkp), value,
                    currency(value));
#else
                You("%s%s��%ld%s�̎؂���������I", still, shkname(shkp), value,
                    currency(value));
#endif
            else
#if 0 /*JP*/
                You("%sowe %s %ld %s for %s!", still, shkname(shkp),
                    value, currency(value), (obj->quan > 1L) ? "them" : "it");
#else
                You("%s%s��%ld%s�̎؂���������I", still, mon_nam(shkp), value,
                    currency(value));
#endif
        }
    } else {
        ESHK(shkp)->robbed += value;

        if (!silent) {
            if (canseemon(shkp)) {
#if 0 /*JP*/
                Norep("%s booms: \"%s, you are a thief!\"",
                      Shknam(shkp), plname);
#else
                Norep("%s�͋��񂾁F�u%s�C�҂āI���̂ǂ�ڂ��߁I�v",
                      Shknam(shkp),  plname);
#endif
            } else
/*JP
                Norep("You hear a scream, \"Thief!\"");
*/
                Norep("���؂萺�𕷂����C�u�҂āI���̂ǂ�ڂ��߁I�v");
        }
        hot_pursuit(shkp);
        (void) angry_guards(FALSE);
    }
    return value;
}

/* auto-response flag for/from "sell foo?" 'a' => 'y', 'q' => 'n' */
static char sell_response = 'a';
static int sell_how = SELL_NORMAL;
/* can't just use sell_response='y' for auto_credit because the 'a' response
   shouldn't carry over from ordinary selling to credit selling */
static boolean auto_credit = FALSE;

void
sellobj_state(deliberate)
int deliberate;
{
    /* If we're deliberately dropping something, there's no automatic
       response to the shopkeeper's "want to sell" query; however, if we
       accidentally drop anything, the shk will buy it/them without asking.
       This retains the old pre-query risk that slippery fingers while in
       shops entailed:  you drop it, you've lost it.
     */
    sell_response = (deliberate != SELL_NORMAL) ? '\0' : 'a';
    sell_how = deliberate;
    auto_credit = FALSE;
}

void
sellobj(obj, x, y)
register struct obj *obj;
xchar x, y;
{
    register struct monst *shkp;
    register struct eshk *eshkp;
    long ltmp = 0L, cltmp = 0L, gltmp = 0L, offer, shkmoney;
    boolean saleitem, cgold = FALSE, container = Has_contents(obj);
    boolean isgold = (obj->oclass == COIN_CLASS);
    boolean only_partially_your_contents = FALSE;

    if (!*u.ushops) /* do cheapest exclusion test first */
        return;
    if (!(shkp = shop_keeper(*in_rooms(x, y, SHOPBASE))) || !inhishop(shkp))
        return;
    if (!costly_spot(x, y))
        return;

    if (obj->unpaid && !container && !isgold) {
        sub_one_frombill(obj, shkp);
        return;
    }
    if (container) {
        /* find the price of content before subfrombill */
        cltmp = contained_cost(obj, shkp, cltmp, TRUE, FALSE);
        /* find the value of contained gold */
        gltmp += contained_gold(obj);
        cgold = (gltmp > 0L);
    }

    saleitem = saleable(shkp, obj);
    if (!isgold && !obj->unpaid && saleitem)
        ltmp = set_cost(obj, shkp);

    offer = ltmp + cltmp;

    /* get one case out of the way: nothing to sell, and no gold */
    if (!(isgold || cgold)
        && ((offer + gltmp) == 0L || sell_how == SELL_DONTSELL)) {
        boolean unpaid = is_unpaid(obj);

        if (container) {
            dropped_container(obj, shkp, FALSE);
            if (!obj->unpaid)
                obj->no_charge = 1;
            if (unpaid)
                subfrombill(obj, shkp);
        } else
            obj->no_charge = 1;

        if (!unpaid && (sell_how != SELL_DONTSELL)
            && !special_stock(obj, shkp, FALSE))
/*JP
            pline("%s seems uninterested.", Shknam(shkp));
*/
            pline("%s�͋������Ȃ��悤���D", Shknam(shkp));
        return;
    }

    /* you dropped something of your own - probably want to sell it */
    rouse_shk(shkp, TRUE); /* wake up sleeping or paralyzed shk */
    eshkp = ESHK(shkp);

    if (ANGRY(shkp)) { /* they become shop-objects, no pay */
        if (!Deaf && !muteshk(shkp))
/*JP
            verbalize("Thank you, scum!");
*/
            verbalize("���肪�Ƃ�C���̃N�\������I");
        else
/*JP
            pline("%s smirks with satisfaction.", Shknam(shkp));
*/
            pline("%s�͖������č��΂��𕂂��ׂ��D", Shknam(shkp));
        subfrombill(obj, shkp);
        return;
    }

    if (eshkp->robbed) { /* shkp is not angry? */
        if (isgold)
            offer = obj->quan;
        else if (cgold)
            offer += cgold;
        if ((eshkp->robbed -= offer < 0L))
            eshkp->robbed = 0L;
        if (offer && !Deaf && !muteshk(shkp))
            verbalize(
/*JP
  "Thank you for your contribution to restock this recently plundered shop.");
*/
  "�񑡂��ǂ������肪�Ƃ��D�ŋߓ��݂ɂ����ĎQ���Ă��񂾁D");
        subfrombill(obj, shkp);
        return;
    }

    if (isgold || cgold) {
        if (!cgold)
            gltmp = obj->quan;

        if (eshkp->debit >= gltmp) {
            if (eshkp->loan) { /* you carry shop's gold */
                if (eshkp->loan >= gltmp)
                    eshkp->loan -= gltmp;
                else
                    eshkp->loan = 0L;
            }
            eshkp->debit -= gltmp;
/*JP
            Your("debt is %spaid off.", eshkp->debit ? "partially " : "");
*/
            Your("�؋���%s�x����ꂽ�D", eshkp->debit ? "�ꕔ" : "");
        } else {
            long delta = gltmp - eshkp->debit;

            eshkp->credit += delta;
            if (eshkp->debit) {
                eshkp->debit = 0L;
                eshkp->loan = 0L;
/*JP
                Your("debt is paid off.");
*/
                Your("�؋��͎x����ꂽ�D");
            }
            if (eshkp->credit == delta)
#if 0 /*JP*/
                You("have established %ld %s credit.", delta,
                    currency(delta));
#else
                pline("%ld%s��a�����D", delta,
                      currency(delta));
#endif
            else
#if 0 /*JP*/
                pline("%ld %s added to your credit; total is now %ld %s.",
                      delta, currency(delta), eshkp->credit,
                      currency(eshkp->credit));
#else
                pline("�a������%ld%s�ǉ����ꂽ; ���v��%ld%s�ɂȂ����D.",
                      delta, currency(delta), eshkp->credit,
                      currency(eshkp->credit));
#endif
        }

        if (!offer || sell_how == SELL_DONTSELL) {
            if (!isgold) {
                if (container)
                    dropped_container(obj, shkp, FALSE);
                if (!obj->unpaid)
                    obj->no_charge = 1;
                subfrombill(obj, shkp);
            }
            return;
        }
    }

    if ((!saleitem && !(container && cltmp > 0L)) || eshkp->billct == BILLSZ
        || obj->oclass == BALL_CLASS || obj->oclass == CHAIN_CLASS
        || offer == 0L || (obj->oclass == FOOD_CLASS && obj->oeaten)
        || (Is_candle(obj)
            && obj->age < 20L * (long) objects[obj->otyp].oc_cost)) {
#if 0 /*JP:T*/
        pline("%s seems uninterested%s.", Shknam(shkp),
              cgold ? " in the rest" : "");
#else
        pline("%s��%s�������Ȃ��悤���D", Shknam(shkp),
              cgold ? "�c�蕨�ɂ�" : "");
#endif
        if (container)
            dropped_container(obj, shkp, FALSE);
        obj->no_charge = 1;
        return;
    }

    shkmoney = money_cnt(shkp->minvent);
    if (!shkmoney) {
        char c, qbuf[BUFSZ];
#if 1 /*JP*/
        char qsfx[BUFSZ];
#endif
        long tmpcr = ((offer * 9L) / 10L) + (offer <= 1L);

        if (sell_how == SELL_NORMAL || auto_credit) {
            c = sell_response = 'y';
        } else if (sell_response != 'n') {
/*JP
            pline("%s cannot pay you at present.", shkname(shkp));
*/
            pline("%s�͍��̂Ƃ���͎x�����Ȃ��D", Monnam(shkp));
#if 0 /*JP*/
            Sprintf(qbuf, "Will you accept %ld %s in credit for ", tmpcr,
                    currency(tmpcr));
            c = ynaq(safe_qbuf(qbuf, qbuf, "?", obj, doname, thesimpleoname,
                               (obj->quan == 1L) ? "that" : "those"));
#else
            Sprintf(qsfx, "�ɂ���%ld%s�̗a�������󂯂���܂����H", tmpcr,
                    currency(tmpcr));
            c = ynaq(safe_qbuf(qbuf, "", qsfx, obj, doname, thesimpleoname,
                               "����"));
#endif
            if (c == 'a') {
                c = 'y';
                auto_credit = TRUE;
            }
        } else /* previously specified "quit" */
            c = 'n';

        if (c == 'y') {
#if 0 /*JP*/
            shk_names_obj(
                shkp, obj,
                (sell_how != SELL_NORMAL)
                    ? "traded %s for %ld zorkmid%s in %scredit."
                    : "relinquish %s and acquire %ld zorkmid%s in %scredit.",
                tmpcr, (eshkp->credit > 0L) ? "additional " : "");
#else
            shk_names_obj(
                shkp, obj,
                (sell_how != SELL_NORMAL)
                    ? "%s������%ld�����̗a�����Ŏ󂯂Ƃ���%s�D"
                    : "%s��n���C����%ld�����̗a�����𓾂�%s�D",
                tmpcr, "");
#endif
            eshkp->credit += tmpcr;
            subfrombill(obj, shkp);
        } else {
            if (c == 'q')
                sell_response = 'n';
            if (container)
                dropped_container(obj, shkp, FALSE);
            if (!obj->unpaid)
                obj->no_charge = 1;
            subfrombill(obj, shkp);
        }
    } else {
        char qbuf[BUFSZ], qsfx[BUFSZ];
#if 0 /*JP*/
        boolean short_funds = (offer > shkmoney), one;
#else
        boolean short_funds = (offer > shkmoney);
#endif

        if (short_funds)
            offer = shkmoney;
        if (!sell_response) {
            long yourc = 0L, shksc;

            if (container) {
                /* number of items owned by shk */
                shksc = count_contents(obj, TRUE, TRUE, FALSE);
                /* number of items owned by you (total - shksc) */
                yourc = count_contents(obj, TRUE, TRUE, TRUE) - shksc;
                only_partially_your_contents = shksc && yourc;
            }
            /*
               "<shk> offers * for ..." query formatting.
               Normal item(s):
                "... your <object>.  Sell it?"
                "... your <objects>.  Sell them?"
               A container is either owned by the hero, or already
               owned by the shk (!ltmp), or the shk isn't interested
               in buying it (also !ltmp).  It's either empty (!cltmp)
               or it has contents owned by the hero or it has some
               contents owned by the hero and others by the shk.
               (The case where it has contents already entirely owned
               by the shk is treated the same was if it were empty
               since the hero isn't selling any of those contents.)
               Your container:
                "... your <empty bag>.  Sell it?"
                "... your <bag> and its contents.  Sell them?"
                "... your <bag> and item inside.  Sell them?"
                "... your <bag> and items inside.  Sell them?"
               Shk's container:
                "... your item in the <bag>.  Sell it?"
                "... your items in the <bag>.  Sell them?"
             */
#if 0 /*JP*/
            Sprintf(qbuf, "%s offers%s %ld gold piece%s for %s%s ",
                    shkname(shkp), short_funds ? " only" : "", offer,
                    plur(offer),
                    (cltmp && !ltmp)
                        ? ((yourc == 1L) ? "your item in " : "your items in ")
                        : "",
                    obj->unpaid ? "the" : "your");
            one = obj->unpaid ? (yourc == 1L) : (obj->quan == 1L && !cltmp);
            Sprintf(qsfx, "%s.  Sell %s?",
                    (cltmp && ltmp)
                        ? (only_partially_your_contents
                               ? ((yourc == 1L) ? " and item inside"
                                                : " and items inside")
                               : and_its_contents)
                        : "",
                    one ? "it" : "them");
            (void) safe_qbuf(qbuf, qbuf, qsfx, obj, xname, simpleonames,
                             one ? "that" : "those");
#else
            Sprintf(qbuf,
                    "%s�͂��Ȃ���", Monnam(shkp));
            Sprintf(qsfx,
                    "%s��%ld�̒l%s�D����܂����H",
                    (!ltmp && cltmp) ? "�̒��g" : "",
                    offer, 
                    short_funds ? "�������Ȃ�����" : "������");
            (void) safe_qbuf(qbuf, qbuf, qsfx, obj, xname, simpleonames,
                             "����");
#endif
        } else
            qbuf[0] = '\0'; /* just to pacify lint */

        switch (sell_response ? sell_response : ynaq(qbuf)) {
        case 'q':
            sell_response = 'n';
            /*FALLTHRU*/
        case 'n':
            if (container)
                dropped_container(obj, shkp, FALSE);
            if (!obj->unpaid)
                obj->no_charge = 1;
            subfrombill(obj, shkp);
            break;
        case 'a':
            sell_response = 'y';
            /*FALLTHRU*/
        case 'y':
            if (container)
                dropped_container(obj, shkp, TRUE);
            if (!obj->unpaid && !saleitem)
                obj->no_charge = 1;
            subfrombill(obj, shkp);
            pay(-offer, shkp);
#if 0 /*JP*/
            shk_names_obj(shkp, obj,
                          (sell_how != SELL_NORMAL)
                           ? ((!ltmp && cltmp && only_partially_your_contents)
                         ? "sold some items inside %s for %ld gold piece%s.%s"
                         : "sold %s for %ld gold piece%s.%s")
            : "relinquish %s and receive %ld gold piece%s in compensation.%s",
                          offer, "");
#else
            shk_names_obj(shkp, obj,
                          (sell_how != SELL_NORMAL)
                           ? ((!ltmp && cltmp && only_partially_your_contents)
                         ? "%s�̒��g������%ld���Ŕ�����%s�D%s"
                         : "%s������%ld���Ŕ�����%s�D%s")
            : "%s��n���C����%ld���̑㏞���󂯂Ƃ���%s�D%s",
                          offer, "");
#endif
            break;
        default:
            impossible("invalid sell response");
        }
    }
}

int
doinvbill(mode)
int mode; /* 0: deliver count 1: paged */
{
#ifdef __SASC
    void sasc_bug(struct obj *, unsigned);
#endif
    struct monst *shkp;
    struct eshk *eshkp;
    struct bill_x *bp, *end_bp;
    struct obj *obj;
    long totused;
    char *buf_p;
    winid datawin;

    shkp = shop_keeper(*u.ushops);
    if (!shkp || !inhishop(shkp)) {
        if (mode != 0)
            impossible("doinvbill: no shopkeeper?");
        return 0;
    }
    eshkp = ESHK(shkp);

    if (mode == 0) {
        /* count expended items, so that the `I' command can decide
           whether to include 'x' in its prompt string */
        int cnt = !eshkp->debit ? 0 : 1;

        for (bp = eshkp->bill_p, end_bp = &eshkp->bill_p[eshkp->billct];
             bp < end_bp; bp++)
            if (bp->useup
                || ((obj = bp_to_obj(bp)) != 0 && obj->quan < bp->bquan))
                cnt++;
        return cnt;
    }

    datawin = create_nhwindow(NHW_MENU);
/*JP
    putstr(datawin, 0, "Unpaid articles already used up:");
*/
    putstr(datawin, 0, "���łɎg���Ă��܂��������̕i�ځF");
    putstr(datawin, 0, "");

    totused = 0L;
    for (bp = eshkp->bill_p, end_bp = &eshkp->bill_p[eshkp->billct];
         bp < end_bp; bp++) {
        obj = bp_to_obj(bp);
        if (!obj) {
            impossible("Bad shopkeeper administration.");
            goto quit;
        }
        if (bp->useup || bp->bquan > obj->quan) {
            long oquan, uquan, thisused;

            oquan = obj->quan;
            uquan = (bp->useup ? bp->bquan : bp->bquan - oquan);
            thisused = bp->price * uquan;
            totused += thisused;
            iflags.suppress_price++; /* suppress "(unpaid)" suffix */
            /* Why 'x'?  To match `I x', more or less. */
            buf_p = xprname(obj, (char *) 0, 'x', FALSE, thisused, uquan);
            iflags.suppress_price--;
            putstr(datawin, 0, buf_p);
        }
    }
    if (eshkp->debit) {
        /* additional shop debt which has no itemization available */
        if (totused)
            putstr(datawin, 0, "");
        totused += eshkp->debit;
#if 0 /*JP*/
        buf_p = xprname((struct obj *) 0, "usage charges and/or other fees",
                        GOLD_SYM, FALSE, eshkp->debit, 0L);
#else
        buf_p = xprname((struct obj *) 0, "�g�p���܂��͑��̎萔��",
                        GOLD_SYM, FALSE, eshkp->debit, 0L);
#endif
        putstr(datawin, 0, buf_p);
    }
/*JP
    buf_p = xprname((struct obj *) 0, "Total:", '*', FALSE, totused, 0L);
*/
    buf_p = xprname((struct obj *) 0, "���v�F", '*', FALSE, totused, 0L);
    putstr(datawin, 0, "");
    putstr(datawin, 0, buf_p);
    display_nhwindow(datawin, FALSE);
quit:
    destroy_nhwindow(datawin);
    return 0;
}

STATIC_OVL long
getprice(obj, shk_buying)
register struct obj *obj;
boolean shk_buying;
{
    register long tmp = (long) objects[obj->otyp].oc_cost;

    if (obj->oartifact) {
        tmp = arti_cost(obj);
        if (shk_buying)
            tmp /= 4;
    }
    switch (obj->oclass) {
    case FOOD_CLASS:
        /* simpler hunger check, (2-4)*cost */
        if (u.uhs >= HUNGRY && !shk_buying)
            tmp *= (long) u.uhs;
        if (obj->oeaten)
            tmp = 0L;
        break;
    case WAND_CLASS:
        if (obj->spe == -1)
            tmp = 0L;
        break;
    case POTION_CLASS:
        if (obj->otyp == POT_WATER && !obj->blessed && !obj->cursed)
            tmp = 0L;
        break;
    case ARMOR_CLASS:
    case WEAPON_CLASS:
        if (obj->spe > 0)
            tmp += 10L * (long) obj->spe;
        break;
    case TOOL_CLASS:
        if (Is_candle(obj)
            && obj->age < 20L * (long) objects[obj->otyp].oc_cost)
            tmp /= 2L;
        break;
    }
    return tmp;
}

/* shk catches thrown pick-axe */
struct monst *
shkcatch(obj, x, y)
register struct obj *obj;
register xchar x, y;
{
    register struct monst *shkp;

    if (!(shkp = shop_keeper(inside_shop(x, y))) || !inhishop(shkp))
        return 0;

    if (shkp->mcanmove && !shkp->msleeping
        && (*u.ushops != ESHK(shkp)->shoproom || !inside_shop(u.ux, u.uy))
        && dist2(shkp->mx, shkp->my, x, y) < 3
        /* if it is the shk's pos, you hit and anger him */
        && (shkp->mx != x || shkp->my != y)) {
        if (mnearto(shkp, x, y, TRUE) && !Deaf && !muteshk(shkp))
/*JP
            verbalize("Out of my way, scum!");
*/
                    verbalize("�ǂ��C�N�\������I");
        if (cansee(x, y)) {
#if 0 /*JP:T*/
            pline("%s nimbly%s catches %s.", Shknam(shkp),
                  (x == shkp->mx && y == shkp->my) ? "" : " reaches over and",
                  the(xname(obj)));
#else
            pline("%s�͂��΂₭%s%s�����܂����D", Shknam(shkp),
                  (x == shkp->mx && y == shkp->my) ? "" : "�߂Â���",
                  xname(obj));
#endif
            if (!canspotmon(shkp))
                map_invisible(x, y);
            delay_output();
            mark_synch();
        }
        subfrombill(obj, shkp);
        (void) mpickobj(shkp, obj);
        return shkp;
    }
    return (struct monst *) 0;
}

void
add_damage(x, y, cost)
register xchar x, y;
long cost;
{
    struct damage *tmp_dam;
    char *shops;

    if (IS_DOOR(levl[x][y].typ)) {
        struct monst *mtmp;

        /* Don't schedule for repair unless it's a real shop entrance */
        for (shops = in_rooms(x, y, SHOPBASE); *shops; shops++)
            if ((mtmp = shop_keeper(*shops)) != 0 && x == ESHK(mtmp)->shd.x
                && y == ESHK(mtmp)->shd.y)
                break;
        if (!*shops)
            return;
    }
    for (tmp_dam = level.damagelist; tmp_dam; tmp_dam = tmp_dam->next)
        if (tmp_dam->place.x == x && tmp_dam->place.y == y) {
            tmp_dam->cost += cost;
            return;
        }
    tmp_dam = (struct damage *) alloc((unsigned) sizeof(struct damage));
    (void) memset((genericptr_t)tmp_dam, 0, sizeof(struct damage));
    tmp_dam->when = monstermoves;
    tmp_dam->place.x = x;
    tmp_dam->place.y = y;
    tmp_dam->cost = cost;
    tmp_dam->typ = levl[x][y].typ;
    tmp_dam->next = level.damagelist;
    level.damagelist = tmp_dam;
    /* If player saw damage, display as a wall forever */
    if (cansee(x, y))
        levl[x][y].seenv = SVALL;
}

/*
 * Do something about damage. Either (!croaked) try to repair it, or
 * (croaked) just discard damage structs for non-shared locations, since
 * they'll never get repaired. Assume that shared locations will get
 * repaired eventually by the other shopkeeper(s). This might be an erroneous
 * assumption (they might all be dead too), but we have no reasonable way of
 * telling that.
 */
STATIC_OVL
void
remove_damage(shkp, croaked)
struct monst *shkp;
boolean croaked;
{
    struct damage *tmp_dam, *tmp2_dam;
    boolean did_repair = FALSE, saw_door = FALSE, saw_floor = FALSE,
            stop_picking = FALSE, doorway_trap = FALSE;
    int saw_walls = 0, saw_untrap = 0;
    char trapmsg[BUFSZ];

    tmp_dam = level.damagelist;
    tmp2_dam = 0;
    while (tmp_dam) {
        register xchar x = tmp_dam->place.x, y = tmp_dam->place.y;
        char shops[5];
        int disposition;
        unsigned old_doormask = 0;

        disposition = 0;
        Strcpy(shops, in_rooms(x, y, SHOPBASE));
        if (index(shops, ESHK(shkp)->shoproom)) {
            if (IS_DOOR(levl[x][y].typ))
                old_doormask = levl[x][y].doormask;

            if (croaked)
                disposition = (shops[1]) ? 0 : 1;
            else if (stop_picking)
                disposition = repair_damage(shkp, tmp_dam, FALSE);
            else {
                /* Defer the stop_occupation() until after repair msgs */
                if (closed_door(x, y))
                    stop_picking = picking_at(x, y);
                disposition = repair_damage(shkp, tmp_dam, FALSE);
                if (!disposition)
                    stop_picking = FALSE;
            }
        }

        if (!disposition) {
            tmp2_dam = tmp_dam;
            tmp_dam = tmp_dam->next;
            continue;
        }

        if (disposition > 1) {
            did_repair = TRUE;
            if (cansee(x, y)) {
                if (IS_WALL(levl[x][y].typ)) {
                    saw_walls++;
                } else if (IS_DOOR(levl[x][y].typ)
                           /* an existing door here implies trap removal */
                           && !(old_doormask & (D_ISOPEN | D_CLOSED))) {
                    saw_door = TRUE;
                } else if (disposition == 3) { /* untrapped */
                    saw_untrap++;
                    if (IS_DOOR(levl[x][y].typ))
                        doorway_trap = TRUE;
                } else {
                    saw_floor = TRUE;
                }
            }
        }

        tmp_dam = tmp_dam->next;
        if (!tmp2_dam) {
            free((genericptr_t) level.damagelist);
            level.damagelist = tmp_dam;
        } else {
            free((genericptr_t) tmp2_dam->next);
            tmp2_dam->next = tmp_dam;
        }
    }
    if (!did_repair)
        return;

    if (saw_untrap) {
#if 0 /*JP*/
        Sprintf(trapmsg, "%s trap%s",
                (saw_untrap > 3) ? "several" : (saw_untrap > 1) ? "some"
                                                                : "a",
                plur(saw_untrap));
        Sprintf(eos(trapmsg), " %s", vtense(trapmsg, "are"));
        Sprintf(eos(trapmsg), " removed from the %s",
                (doorway_trap && saw_untrap == 1) ? "doorway" : "floor");
#else
        Sprintf(trapmsg, "%s㩂�%s�������",
                (saw_untrap > 3) ? "����������" : (saw_untrap > 1) ? "���J����"
                                                                   : "",
                (doorway_trap && saw_untrap == 1) ? "��" : "��");
#endif
    } else
        trapmsg[0] = '\0'; /* not just lint suppression... */

    if (saw_walls) {
#if 0 /*JP*/
        char wallbuf[BUFSZ];

        Sprintf(wallbuf, "section%s", plur(saw_walls));
        pline("Suddenly, %s %s of wall %s up!",
              (saw_walls == 1) ? "a" : (saw_walls <= 3) ? "some" : "several",
              wallbuf, vtense(wallbuf, "close"));
#else
        pline("�ˑR�C�ǂ�%s�܂����I",
              (saw_walls == 1) ? "" : (saw_walls <= 3) ? "���ӏ���" : "����������");
#endif

        if (saw_door)
/*JP
            pline_The("shop door reappears!");
*/
            pline("�X�̔����܂�����ꂽ�I");
        if (saw_floor)
/*JP
            pline_The("floor is repaired!");
*/
            pline("���͏C�����ꂽ�I");
        if (saw_untrap)
/*JP
            pline("%s!", upstart(trapmsg));
*/
            pline("%s���I", trapmsg);
    } else {
        if (saw_door || saw_floor || saw_untrap)
#if 0 /*JP*/
            pline("Suddenly, %s%s%s%s%s!",
                  saw_door ? "the shop door reappears" : "",
                  (saw_door && saw_floor) ? " and " : "",
                  saw_floor ? "the floor damage is gone" : "",
                  ((saw_door || saw_floor) && *trapmsg) ? " and " : "",
                  trapmsg);
#else
            pline("�ˑR�C%s%s%s%s%s��!",
                  saw_door ? "�X�̔����܂������" : "",
                  (saw_door && saw_floor) ? "�C" : "",
                  saw_floor ? "���̏����C������" : "",
                  ((saw_door || saw_floor) && *trapmsg) ? "�C" : "",
                  trapmsg);
#endif
        else if (inside_shop(u.ux, u.uy) == ESHK(shkp)->shoproom)
/*JP
            You_feel("more claustrophobic than before.");
*/
            You("�O�������|�ǋC���ɂȂ����D");
        else if (!Deaf && !rn2(10))
/*JP
            Norep("The dungeon acoustics noticeably change.");
*/
            Norep("���{�̉����͂������邵���ς����D");
    }
    if (stop_picking)
        stop_occupation();
}

/*
 * 0: repair postponed, 1: silent repair (no messages), 2: normal repair
 * 3: untrap
 */
int
repair_damage(shkp, tmp_dam, catchup)
register struct monst *shkp;
register struct damage *tmp_dam;
boolean catchup; /* restoring a level */
{
    register xchar x, y, i;
    xchar litter[9];
    register struct monst *mtmp;
    register struct obj *otmp;
    register struct trap *ttmp;

    if ((monstermoves - tmp_dam->when) < REPAIR_DELAY)
        return 0;
    if (shkp->msleeping || !shkp->mcanmove || ESHK(shkp)->following)
        return 0;
    x = tmp_dam->place.x;
    y = tmp_dam->place.y;
    if (!IS_ROOM(tmp_dam->typ)) {
        if (x == u.ux && y == u.uy)
            if (!Passes_walls)
                return 0;
        if (x == shkp->mx && y == shkp->my)
            return 0;
        if ((mtmp = m_at(x, y)) && (!passes_walls(mtmp->data)))
            return 0;
    }
    if ((ttmp = t_at(x, y)) != 0) {
        if (x == u.ux && y == u.uy)
            if (!Passes_walls)
                return 0;
        if (ttmp->ttyp == LANDMINE || ttmp->ttyp == BEAR_TRAP) {
            /* convert to an object */
            otmp = mksobj((ttmp->ttyp == LANDMINE) ? LAND_MINE : BEARTRAP,
                          TRUE, FALSE);
            otmp->quan = 1L;
            otmp->owt = weight(otmp);
            (void) mpickobj(shkp, otmp);
        }
        deltrap(ttmp);
        if (IS_DOOR(tmp_dam->typ) && !(levl[x][y].doormask & D_ISOPEN)) {
            levl[x][y].doormask = D_CLOSED;
            block_point(x, y);
        } else if (IS_WALL(tmp_dam->typ)) {
            levl[x][y].typ = tmp_dam->typ;
            block_point(x, y);
        }
        newsym(x, y);
        return 3;
    }
    if (IS_ROOM(tmp_dam->typ)) {
        /* No messages, because player already filled trap door */
        return 1;
    }
    if ((tmp_dam->typ == levl[x][y].typ)
        && (!IS_DOOR(tmp_dam->typ) || (levl[x][y].doormask > D_BROKEN)))
        /* No messages if player already replaced shop door */
        return 1;
    levl[x][y].typ = tmp_dam->typ;
    (void) memset((genericptr_t) litter, 0, sizeof(litter));
    if ((otmp = level.objects[x][y]) != 0) {
/* Scatter objects haphazardly into the shop */
#define NEED_UPDATE 1
#define OPEN 2
#define INSHOP 4
#define horiz(i) ((i % 3) - 1)
#define vert(i) ((i / 3) - 1)
        for (i = 0; i < 9; i++) {
            if ((i == 4) || (!ZAP_POS(levl[x + horiz(i)][y + vert(i)].typ)))
                continue;
            litter[i] = OPEN;
            if (inside_shop(x + horiz(i), y + vert(i))
                == ESHK(shkp)->shoproom)
                litter[i] |= INSHOP;
        }
        if (Punished && !u.uswallow
            && ((uchain->ox == x && uchain->oy == y)
                || (uball->ox == x && uball->oy == y))) {
            /*
             * Either the ball or chain is in the repair location.
             *
             * Take the easy way out and put ball&chain under hero.
             */
            if (!Deaf && !muteshk(shkp))
/*JP
                verbalize("Get your junk out of my wall!");
*/
                verbalize("���̃K���N�^���O�Ɏ����čs���ȁI");
            unplacebc(); /* pick 'em up */
            placebc();   /* put 'em down */
        }
        while ((otmp = level.objects[x][y]) != 0)
            /* Don't mess w/ boulders -- just merge into wall */
            if ((otmp->otyp == BOULDER) || (otmp->otyp == ROCK)) {
                obj_extract_self(otmp);
                obfree(otmp, (struct obj *) 0);
            } else {
                while (!(litter[i = rn2(9)] & INSHOP))
                    ;
                remove_object(otmp);
                place_object(otmp, x + horiz(i), y + vert(i));
                litter[i] |= NEED_UPDATE;
            }
    }
    if (catchup)
        return 1; /* repair occurred while off level */

    block_point(x, y);
    if (IS_DOOR(tmp_dam->typ)) {
        levl[x][y].doormask = D_CLOSED; /* arbitrary */
        newsym(x, y);
    } else {
        /* don't set doormask  - it is (hopefully) the same as it was
           if not, perhaps save it with the damage array... */

        if (IS_WALL(tmp_dam->typ) && cansee(x, y)) {
            /* Player sees actual repair process, so they KNOW it's a wall */
            levl[x][y].seenv = SVALL;
            newsym(x, y);
        }
        /* Mark this wall as "repaired".  There currently is no code
           to do anything about repaired walls, so don't do it. */
    }
    for (i = 0; i < 9; i++)
        if (litter[i] & NEED_UPDATE)
            newsym(x + horiz(i), y + vert(i));
    return 2;
#undef NEED_UPDATE
#undef OPEN
#undef INSHOP
#undef vert
#undef horiz
}

/*
 * shk_move: return 1: moved  0: didn't  -1: let m_move do it  -2: died
 */
int
shk_move(shkp)
register struct monst *shkp;
{
    register xchar gx, gy, omx, omy;
    register int udist;
    register schar appr;
    register struct eshk *eshkp = ESHK(shkp);
    int z;
    boolean uondoor = FALSE, satdoor, avoid = FALSE, badinv;

    omx = shkp->mx;
    omy = shkp->my;

    if (inhishop(shkp))
        remove_damage(shkp, FALSE);

    if ((udist = distu(omx, omy)) < 3 && (shkp->data != &mons[PM_GRID_BUG]
                                          || (omx == u.ux || omy == u.uy))) {
        if (ANGRY(shkp) || (Conflict && !resist(shkp, RING_CLASS, 0, 0))) {
            if (Displaced)
/*JP
                Your("displaced image doesn't fool %s!", shkname(shkp));
*/
                pline("%s�͂��Ȃ��̌��e�ɂ��܂���Ȃ������I", shkname(shkp));
            (void) mattacku(shkp);
            return 0;
        }
        if (eshkp->following) {
            if (strncmp(eshkp->customer, plname, PL_NSIZ)) {
                if (!Deaf && !muteshk(shkp))
#if 0 /*JP:T*/
                    verbalize("%s, %s!  I was looking for %s.", Hello(shkp),
                              plname, eshkp->customer);
#else
                    verbalize("%s�I�킽����%s��T���Ă��܂��D", Hello(shkp),
                              eshkp->customer);
#endif
                eshkp->following = 0;
                return 0;
            }
            if (moves > followmsg + 4) {
                if (!Deaf && !muteshk(shkp))
#if 0 /*JP:T*/
                    verbalize("%s, %s!  Didn't you forget to pay?",
                              Hello(shkp), plname);
#else
                    verbalize("%s�I�x������Y��Ă��܂��񂩁H",
                              Hello(shkp));
#endif
                else
#if 0 /*JP*/
                    pline("%s holds out %s upturned %s.",
                          Shknam(shkp),
                          mhis(shkp),
                          mbodypart(shkp, HAND));
#else
                    pline("%s��%s����Ɍ����č����o�����D",
                          Shknam(shkp),
                          mbodypart(shkp, HAND));
#endif
                followmsg = moves;
                if (!rn2(9)) {
#if 0 /*JP*/
                    pline("%s doesn't like customers who don't pay.",
                          Shknam(shkp));
#else
                    pline("%s�͋��𕥂�Ȃ��q�������݂������D",
                          Shknam(shkp));
#endif
                    rile_shk(shkp);
                }
            }
            if (udist < 2)
                return 0;
        }
    }

    appr = 1;
    gx = eshkp->shk.x;
    gy = eshkp->shk.y;
    satdoor = (gx == omx && gy == omy);
    if (eshkp->following || ((z = holetime()) >= 0 && z * z <= udist)) {
        /* [This distance check used to apply regardless of
            whether the shk was following, but that resulted in
            m_move() sometimes taking the shk out of the shop if
            the player had fenced him in with boulders or traps.
            Such voluntary abandonment left unpaid objects in
            invent, triggering billing impossibilities on the
            next level once the character fell through the hole.] */
        if (udist > 4 && eshkp->following && !eshkp->billct)
            return -1; /* leave it to m_move */
        gx = u.ux;
        gy = u.uy;
    } else if (ANGRY(shkp)) {
        /* Move towards the hero if the shopkeeper can see him. */
        if (shkp->mcansee && m_canseeu(shkp)) {
            gx = u.ux;
            gy = u.uy;
        }
        avoid = FALSE;
    } else {
#define GDIST(x, y) (dist2(x, y, gx, gy))
        if (Invis || u.usteed) {
            avoid = FALSE;
        } else {
            uondoor = (u.ux == eshkp->shd.x && u.uy == eshkp->shd.y);
            if (uondoor) {
                badinv =
                    (carrying(PICK_AXE) || carrying(DWARVISH_MATTOCK)
                     || (Fast && (sobj_at(PICK_AXE, u.ux, u.uy)
                                  || sobj_at(DWARVISH_MATTOCK, u.ux, u.uy))));
                if (satdoor && badinv)
                    return 0;
                avoid = !badinv;
            } else {
                avoid = (*u.ushops && distu(gx, gy) > 8);
                badinv = FALSE;
            }

            if (((!eshkp->robbed && !eshkp->billct && !eshkp->debit) || avoid)
                && GDIST(omx, omy) < 3) {
                if (!badinv && !onlineu(omx, omy))
                    return 0;
                if (satdoor)
                    appr = gx = gy = 0;
            }
        }
    }

    z = move_special(shkp, inhishop(shkp), appr, uondoor, avoid, omx, omy, gx,
                     gy);
    if (z > 0)
        after_shk_move(shkp);

    return z;
}

/* called after shopkeeper moves, in case themove causes re-entry into shop */
void
after_shk_move(shkp)
struct monst *shkp;
{
    struct eshk *eshkp = ESHK(shkp);

    if (eshkp->bill_p == (struct bill_x *) -1000 && inhishop(shkp)) {
        /* reset bill_p, need to re-calc player's occupancy too */
        eshkp->bill_p = &eshkp->bill[0];
        check_special_room(FALSE);
    }
}

/* for use in levl_follower (mondata.c) */
boolean
is_fshk(mtmp)
register struct monst *mtmp;
{
    return (boolean) (mtmp->isshk && ESHK(mtmp)->following);
}

/* You are digging in the shop. */
void
shopdig(fall)
register int fall;
{
    register struct monst *shkp = shop_keeper(*u.ushops);
    int lang;
/*JP
    const char *grabs = "grabs";
*/
    const char *grabs = "����";

    if (!shkp)
        return;

    /* 0 == can't speak, 1 == makes animal noises, 2 == speaks */
    lang = 0;
    if (shkp->msleeping || !shkp->mcanmove || is_silent(shkp->data))
        ; /* lang stays 0 */
    else if (shkp->data->msound <= MS_ANIMAL)
        lang = 1;
    else if (shkp->data->msound >= MS_HUMANOID)
        lang = 2;

    if (!inhishop(shkp)) {
        if (Role_if(PM_KNIGHT)) {
/*JP
            You_feel("like a common thief.");
*/
            You("�D�_���R�ȋC�������D");
            adjalign(-sgn(u.ualign.type));
        }
        return;
    }

    if (!fall) {
        if (lang == 2) {
            if (!Deaf && !muteshk(shkp)) {
                if (u.utraptype == TT_PIT)
#if 0 /*JP*/
                    verbalize(
                        "Be careful, %s, or you might fall through the floor.",
                        flags.female ? "madam" : "sir");
#else
                    verbalize(
                        "���ӂ��Ă�������%s�C�����痎���܂���D",
                        flags.female ? "���삳��" : "���q����");
#endif
                else
#if 0 /*JP*/
                    verbalize("%s, do not damage the floor here!",
                        flags.female ? "Madam" : "Sir");
#else
                    verbalize("%s�C���ɏ������Ȃ��ł��������I",
                        flags.female ? "���삳��" : "���q����");
#endif
            }
        }
        if (Role_if(PM_KNIGHT)) {
/*JP
            You_feel("like a common thief.");
*/
            You("�D�_���R�ȋC�������D");
            adjalign(-sgn(u.ualign.type));
        }
    } else if (!um_dist(shkp->mx, shkp->my, 5)
               && !shkp->msleeping && shkp->mcanmove
               && (ESHK(shkp)->billct || ESHK(shkp)->debit)) {
        register struct obj *obj, *obj2;

        if (nolimbs(shkp->data)) {
/*JP
            grabs = "knocks off";
*/
            grabs = "�@��������";
#if 0
            /* This is what should happen, but for balance
             * reasons, it isn't currently.
             */
            if (lang == 2)
                pline("%s curses %s inability to grab your backpack!",
                      shkname(shkp), mhim(shkp));
            rile_shk(shkp);
            return;
#endif
        }
        if (distu(shkp->mx, shkp->my) > 2) {
            mnexto(shkp);
            /* for some reason the shopkeeper can't come next to you */
            if (distu(shkp->mx, shkp->my) > 2) {
                if (lang == 2)

/*JP
                    pline("%s curses you in anger and frustration!",
*/
                    pline("�{��ŕs���̂��܂��Ă���%s�͂��Ȃ���������I",
                          shkname(shkp));
                else if (lang == 1)
                    growl(shkp);
                rile_shk(shkp);
                return;
            } else
#if 0 /*JP*/
                pline("%s %s, and %s your backpack!", shkname(shkp),
                      makeplural(locomotion(shkp->data, "leap")), grabs);
#else
                pline("%s�͔�т��āC���Ȃ��̔w�����܂�%s�I", shkname(shkp),
                      grabs);
#endif
        } else
/*JP
            pline("%s %s your backpack!", shkname(shkp), grabs);
*/
            pline("%s�͂��Ȃ��̔w�����܂�%s�I", shkname(shkp), grabs);

        for (obj = invent; obj; obj = obj2) {
            obj2 = obj->nobj;
            if ((obj->owornmask & ~(W_SWAPWEP | W_QUIVER)) != 0
                || (obj == uswapwep && u.twoweap)
                || (obj->otyp == LEASH && obj->leashmon))
                continue;
            if (obj == current_wand)
                continue;
            setnotworn(obj);
            freeinv(obj);
            subfrombill(obj, shkp);
            (void) add_to_minv(shkp, obj); /* may free obj */
        }
    }
}

STATIC_OVL void
makekops(mm)
coord *mm;
{
    static const short k_mndx[4] = { PM_KEYSTONE_KOP, PM_KOP_SERGEANT,
                                     PM_KOP_LIEUTENANT, PM_KOP_KAPTAIN };
    int k_cnt[4], cnt, mndx, k;

    k_cnt[0] = cnt = abs(depth(&u.uz)) + rnd(5);
    k_cnt[1] = (cnt / 3) + 1; /* at least one sarge */
    k_cnt[2] = (cnt / 6);     /* maybe a lieutenant */
    k_cnt[3] = (cnt / 9);     /* and maybe a kaptain */

    for (k = 0; k < 4; k++) {
        if ((cnt = k_cnt[k]) == 0)
            break;
        mndx = k_mndx[k];
        if (mvitals[mndx].mvflags & G_GONE)
            continue;

        while (cnt--)
            if (enexto(mm, mm->x, mm->y, &mons[mndx]))
                (void) makemon(&mons[mndx], mm->x, mm->y, NO_MM_FLAGS);
    }
}

void
pay_for_damage(dmgstr, cant_mollify)
const char *dmgstr;
boolean cant_mollify;
{
    register struct monst *shkp = (struct monst *) 0;
    char shops_affected[5];
    register boolean uinshp = (*u.ushops != '\0');
    char qbuf[80];
    register xchar x, y;
#if 0 /*JP*/
    boolean dugwall = (!strcmp(dmgstr, "dig into")    /* wand */
                       || !strcmp(dmgstr, "damage")); /* pick-axe */
#else
    boolean ni_wo = !strcmp(dmgstr, "����������");              /* wand */
    boolean dugwall = (ni_wo || !strcmp(dmgstr, "������")); /* pick-axe */
#endif
    boolean animal, pursue;
    struct damage *tmp_dam, *appear_here = 0;
    /* any number >= (80*80)+(24*24) would do, actually */
    long cost_of_damage = 0L;
    unsigned int nearest_shk = 7000, nearest_damage = 7000;
    int picks = 0;

    for (tmp_dam = level.damagelist;
         (tmp_dam && (tmp_dam->when == monstermoves));
         tmp_dam = tmp_dam->next) {
        char *shp;

        if (!tmp_dam->cost)
            continue;
        cost_of_damage += tmp_dam->cost;
        Strcpy(shops_affected,
               in_rooms(tmp_dam->place.x, tmp_dam->place.y, SHOPBASE));
        for (shp = shops_affected; *shp; shp++) {
            struct monst *tmp_shk;
            unsigned int shk_distance;

            if (!(tmp_shk = shop_keeper(*shp)))
                continue;
            if (tmp_shk == shkp) {
                unsigned int damage_distance =
                    distu(tmp_dam->place.x, tmp_dam->place.y);

                if (damage_distance < nearest_damage) {
                    nearest_damage = damage_distance;
                    appear_here = tmp_dam;
                }
                continue;
            }
            if (!inhishop(tmp_shk))
                continue;
            shk_distance = distu(tmp_shk->mx, tmp_shk->my);
            if (shk_distance > nearest_shk)
                continue;
            if ((shk_distance == nearest_shk) && picks) {
                if (rn2(++picks))
                    continue;
            } else
                picks = 1;
            shkp = tmp_shk;
            nearest_shk = shk_distance;
            appear_here = tmp_dam;
            nearest_damage = distu(tmp_dam->place.x, tmp_dam->place.y);
        }
    }

    if (!cost_of_damage || !shkp)
        return;

    animal = (shkp->data->msound <= MS_ANIMAL);
    pursue = FALSE;
    x = appear_here->place.x;
    y = appear_here->place.y;

    /* not the best introduction to the shk... */
    (void) strncpy(ESHK(shkp)->customer, plname, PL_NSIZ);

    /* if the shk is already on the war path, be sure it's all out */
    if (ANGRY(shkp) || ESHK(shkp)->following) {
        hot_pursuit(shkp);
        return;
    }

    /* if the shk is not in their shop.. */
    if (!*in_rooms(shkp->mx, shkp->my, SHOPBASE)) {
        if (!cansee(shkp->mx, shkp->my))
            return;
        pursue = TRUE;
        goto getcad;
    }

    if (uinshp) {
        if (um_dist(shkp->mx, shkp->my, 1)
            && !um_dist(shkp->mx, shkp->my, 3)) {
/*JP
            pline("%s leaps towards you!", shkname(shkp));
*/
            pline("%s�͂��Ȃ��ɔ�т��������I", shkname(shkp));
            mnexto(shkp);
        }
        pursue = um_dist(shkp->mx, shkp->my, 1);
        if (pursue)
            goto getcad;
    } else {
        /*
         * Make shkp show up at the door.  Effect:  If there is a monster
         * in the doorway, have the hero hear the shopkeeper yell a bit,
         * pause, then have the shopkeeper appear at the door, having
         * yanked the hapless critter out of the way.
         */
        if (MON_AT(x, y)) {
            if (!animal) {
                if (!Deaf && !muteshk(shkp)) {
/*JP
                You_hear("an angry voice:");
*/
                You("�{��̐��𕷂����F");
/*JP
                verbalize("Out of my way, scum!");
*/
                verbalize("�ǂ��I�N�\������I");
                }
                wait_synch();
#if defined(UNIX) || defined(VMS)
#if defined(SYSV) || defined(ULTRIX) || defined(VMS)
                (void)
#endif
                    sleep(1);
#endif
            } else {
                growl(shkp);
            }
        }
        (void) mnearto(shkp, x, y, TRUE);
    }

    if ((um_dist(x, y, 1) && !uinshp) || cant_mollify
        || (money_cnt(invent) + ESHK(shkp)->credit) < cost_of_damage
        || !rn2(50)) {
    getcad:
        if (muteshk(shkp)) {
            if (animal && shkp->mcanmove && !shkp->msleeping)
                yelp(shkp);
        } else if (pursue || uinshp || !um_dist(x, y, 1)) {
            if (!Deaf)
#if 0 /*JP*/
                verbalize("How dare you %s my %s?", dmgstr,
                          dugwall ? "shop" : "door");
#else
                verbalize("�ǂ�����%s%s%s�肵���񂾁H",
                          dugwall ? "�X" : "��",
                          ni_wo ? "��" : "��",
                          jpast(dmgstr));
#endif
            else
#if 0 /*JP*/
                pline("%s is %s that you decided to %s %s %s!",
                      Shknam(shkp), angrytexts[rn2(SIZE(angrytexts))],
                      dmgstr, mhis(shkp), dugwall ? "shop" : "door");
#else
                pline("%s�͂��Ȃ���%s��%s���Ƃɑ΂���%s�I",
                      Shknam(shkp),
                      dugwall ? "�X" : "��", dmgstr,
                      angrytexts[rn2(SIZE(angrytexts))]);
#endif
        } else {
            if (!Deaf) {
/*JP
            pline("%s shouts:", shkname(shkp));
*/
            pline("%s�͂����񂾁F", shkname(shkp));
#if 0 /*JP*/
                verbalize("Who dared %s my %s?", dmgstr,
                          dugwall ? "shop" : "door");
#else
                verbalize("�N��%s%s%s�肵���񂾂낤�H",
                          dugwall ? "�X" : "��",
                          ni_wo ? "��" : "��",
                          jpast(dmgstr));
#endif
            } else {
#if 0 /*JP*/
                pline("%s is %s that someone decided to %s %s %s!",
                      Shknam(shkp), angrytexts[rn2(SIZE(angrytexts))],
                      dmgstr, mhis(shkp), dugwall ? "shop" : "door");
#else
                pline("%s�͒N����%s��%s���Ƃɑ΂���%s�I",
                      Shknam(shkp),
                      dugwall ? "�X" : "��", dmgstr,
                      angrytexts[rn2(SIZE(angrytexts))]);
#endif
            }
        }
        hot_pursuit(shkp);
        return;
    }

    if (Invis)
/*JP
        Your("invisibility does not fool %s!", shkname(shkp));
*/
        pline("%s�͓����Ȃ��Ȃ��ɂ��܂���Ȃ������I", shkname(shkp));
#if 0 /*JP*/
    Sprintf(qbuf, "%sYou did %ld %s worth of damage!%s  Pay?",
            !animal ? cad(TRUE) : "", cost_of_damage,
            currency(cost_of_damage), !animal ? "\"" : "");
#else
    Sprintf(qbuf,"�u�����I%ld%s�̑��Q���I�v�����܂����H",
            cost_of_damage, currency(cost_of_damage));
#endif
    if (yn(qbuf) != 'n') {
        cost_of_damage = check_credit(cost_of_damage, shkp);
        money2mon(shkp, cost_of_damage);
        context.botl = 1;
/*JP
        pline("Mollified, %s accepts your restitution.", shkname(shkp));
*/
        pline("%s�́C��������炰���������󂯂Ƃ����D", shkname(shkp));
        /* move shk back to his home loc */
        home_shk(shkp, FALSE);
        pacify_shk(shkp);
    } else {
        if (!animal) {
            if (!Deaf && !muteshk(shkp))
/*JP
            verbalize("Oh, yes!  You'll pay!");
*/
            verbalize("�����C�����񂾁I");
            else
#if 0 /*JP*/
                pline("%s lunges %s %s toward your %s!",
                    Shknam(shkp),
                    mhis(shkp),
                    mbodypart(shkp, HAND),
                    body_part(NECK));
#else
                pline("%s��%s�����Ȃ���%s�ɓ˂��o�����I",
                    Shknam(shkp),
                    mbodypart(shkp, HAND),
                    body_part(NECK));
#endif
        } else
            growl(shkp);
        hot_pursuit(shkp);
        adjalign(-sgn(u.ualign.type));
    }
}

/* called in dokick.c when we kick an object that might be in a store */
boolean
costly_spot(x, y)
register xchar x, y;
{
    struct monst *shkp;
    struct eshk *eshkp;

    if (!level.flags.has_shop)
        return FALSE;
    shkp = shop_keeper(*in_rooms(x, y, SHOPBASE));
    if (!shkp || !inhishop(shkp))
        return FALSE;
    eshkp = ESHK(shkp);
    return  (boolean) (inside_shop(x, y)
                       && !(x == eshkp->shk.x && y == eshkp->shk.y));
}

/* called by dotalk(sounds.c) when #chatting; returns obj if location
   contains shop goods and shopkeeper is willing & able to speak */
struct obj *
shop_object(x, y)
register xchar x, y;
{
    register struct obj *otmp;
    register struct monst *shkp;

    if (!(shkp = shop_keeper(*in_rooms(x, y, SHOPBASE))) || !inhishop(shkp))
        return (struct obj *) 0;

    for (otmp = level.objects[x][y]; otmp; otmp = otmp->nexthere)
        if (otmp->oclass != COIN_CLASS)
            break;
    /* note: otmp might have ->no_charge set, but that's ok */
    return (otmp && costly_spot(x, y)
            && NOTANGRY(shkp) && shkp->mcanmove && !shkp->msleeping)
               ? otmp
               : (struct obj *) 0;
}

/* give price quotes for all objects linked to this one (ie, on this spot) */
void
price_quote(first_obj)
register struct obj *first_obj;
{
    register struct obj *otmp;
    char buf[BUFSZ], price[40];
    long cost = 0L;
    int cnt = 0;
    boolean contentsonly = FALSE;
    winid tmpwin;
    struct monst *shkp = shop_keeper(inside_shop(u.ux, u.uy));

    tmpwin = create_nhwindow(NHW_MENU);
/*JP
    putstr(tmpwin, 0, "Fine goods for sale:");
*/
    putstr(tmpwin, 0, "����ɂłĂ��邷�΂炵�����i�F");
    putstr(tmpwin, 0, "");
    for (otmp = first_obj; otmp; otmp = otmp->nexthere) {
        if (otmp->oclass == COIN_CLASS)
            continue;
        cost = (otmp->no_charge || otmp == uball || otmp == uchain)
                   ? 0L
                   : get_cost(otmp, (struct monst *) 0);
        contentsonly = !cost;
        if (Has_contents(otmp))
            cost += contained_cost(otmp, shkp, 0L, FALSE, FALSE);
        if (!cost) {
/*JP
            Strcpy(price, "no charge");
*/
            Strcpy(price, "����");
            contentsonly = FALSE;
        } else {
#if 0 /*JP*/
            Sprintf(price, "%ld %s%s", cost, currency(cost),
                    (otmp->quan) > 1L ? " each" : "");
#else
            Sprintf(price, "%s%ld%s", 
                    otmp->quan > 1L ? "���ꂼ��" : "", cost, currency(cost));
#endif
        }
#if 0 /*JP*/
        Sprintf(buf, "%s%s, %s", contentsonly ? the_contents_of : "",
                doname(otmp), price);
#else
        Sprintf(buf, "%s%s�C%s", doname(otmp),
                contentsonly ? "�̒��g" : "", price);
#endif
        putstr(tmpwin, 0, buf), cnt++;
    }
    if (cnt > 1) {
        display_nhwindow(tmpwin, TRUE);
    } else if (cnt == 1) {
        if (!cost) {
            /* "<doname(obj)>, no charge" */
#if 0 /*JP*/
            pline("%s!", upstart(buf)); /* buf still contains the string */
#else
            pline("%s�I", buf); /* buf still contains the string */
#endif
        } else {
            /* print cost in slightly different format, so can't reuse buf;
               cost and contentsonly are already set up */
#if 0 /*JP*/
            Sprintf(buf, "%s%s", contentsonly ? the_contents_of : "",
                    doname(first_obj));
#else
            Sprintf(buf, "%s%s", doname(first_obj),
                    contentsonly ? "�̒��g" : "");
#endif
#if 0 /*JP*/
            pline("%s, price %ld %s%s%s", upstart(buf), cost, currency(cost),
                  (first_obj->quan > 1L) ? " each" : "",
                  contentsonly ? "." : shk_embellish(first_obj, cost));
#else
            pline("%s%s��%s%ld%s���D", 
                  shk_embellish(first_obj, cost), buf,
                  first_obj->quan > 1L ? "���ꂼ��" : "",
                  cost, currency(cost));
#endif
        }
    }
    destroy_nhwindow(tmpwin);
}

/*JP
 �X��̔��蕶��B�p��ł͖����ɕt�������{��ł͐擪�ɕt���B
 ����ɃA�C�e����������̂ŁA�A�C�e���ɑ΂���`�e�ł��Ɨ��������ł��ǂ��B
 */
STATIC_OVL const char *
shk_embellish(itm, cost)
register struct obj *itm;
long cost;
{
    if (!rn2(3)) {
        register int o, choice = rn2(5);

        if (choice == 0)
            choice = (cost < 100L ? 1 : cost < 500L ? 2 : 3);
        switch (choice) {
        case 4:
            if (cost < 10L)
                break;
            else
                o = itm->oclass;
            if (o == FOOD_CLASS)
/*JP
                return ", gourmets' delight!";
*/
                return "�O�����������Ċ��";
            if (objects[itm->otyp].oc_name_known
                    ? objects[itm->otyp].oc_magic
                    : (o == AMULET_CLASS || o == RING_CLASS || o == WAND_CLASS
                       || o == POTION_CLASS || o == SCROLL_CLASS
                       || o == SPBOOK_CLASS))
/*JP
                return ", painstakingly developed!";
*/
                return "�ꋉ�̖��͂��߂�";
/*JP
            return ", superb craftsmanship!";
*/
            return "�ꗬ�E�l�̍����";
        case 3:
/*JP
            return ", finest quality.";
*/
            return "�ō��̕i�����ւ�";
        case 2:
/*JP
            return ", an excellent choice.";
*/
            return "���������q����ڂ������I";
        case 1:
/*JP
            return ", a real bargain.";
*/
            return "�{���̖ڋʏ��i�I";
        default:
            break;
        }
    } else if (itm->oartifact) {
/*JP
        return ", one of a kind!";
*/
        return "����͐��E�ɂ܂��ƂȂ��I";
    }
/*JP
    return ".";
*/
    return "";
}

/* First 4 supplied by Ronen and Tamar, remainder by development team */
const char *Izchak_speaks[] = {
/*JP
    "%s says: 'These shopping malls give me a headache.'",
*/
    "%s�H���w�����V���b�s���O�X�͓��ɂ̃^�l���D�x",
/*JP
    "%s says: 'Slow down.  Think clearly.'",
*/
    "%s�H���w���������āC�����ɍl����D�x",
/*JP
    "%s says: 'You need to take things one at a time.'",
*/
    "%s�H���w��x�Ɉ�����K�v������D�x",
/*JP
    "%s says: 'I don't like poofy coffee... give me Columbian Supremo.'",
*/
    "%s�H���w�z�����ۂ��R�[�q�͍D������Ȃ��D�D�D�R�����r�A���E�X�v���������̂ށD�x",
/*JP
    "%s says that getting the devteam's agreement on anything is difficult.",
*/
    "�J���`�[���ɉ��炩�̋�������߂邱�Ƃ͍����%s�͏q�ׂ��D",
/*JP
    "%s says that he has noticed those who serve their deity will prosper.",
*/
    "%s�͐_�Ɏd���Ă����͐������Ă���Əq�ׂ��D",
/*JP
    "%s says: 'Don't try to steal from me - I have friends in high places!'",
*/
    "%s�H���w�����瓐�����Ȃ�Ďv��Ȃ����Ƃ��D��̕��ɒm�荇��������񂾂���D�x",
/*JP
    "%s says: 'You may well need something from this shop in the future.'",
*/
    "%s�H���w�������̓X�̉������K�v�ƂȂ邾�낤�x",
/*JP
    "%s comments about the Valley of the Dead as being a gateway."
*/
    "%s�͎��̒J�̓Q�[�g�E�F�C���낤�ƃR�����g���q�ׂ��D"
};

void
shk_chat(shkp)
struct monst *shkp;
{
    struct eshk *eshk;
    long shkmoney;

    if (!shkp->isshk) {
        /* The monster type is shopkeeper, but this monster is
           not actually a shk, which could happen if someone
           wishes for a shopkeeper statue and then animates it.
           (Note: shkname() would be "" in a case like this.) */
#if 0 /*JP*/
        pline("%s asks whether you've seen any untended shops recently.",
#else
        pline("%s�͍ŋ߂ǂ����ɓX���̂��Ȃ��X�͂Ȃ����Ƃ����˂��D",
#endif
              Monnam(shkp));
        /* [Perhaps we ought to check whether this conversation
           is taking place inside an untended shop, but a shopless
           shk can probably be expected to be rather disoriented.] */
        return;
    }

    eshk = ESHK(shkp);
    if (ANGRY(shkp)) {
#if 0 /*JP*/
        pline("%s %s how much %s dislikes %s customers.",
              shkname(shkp),
              (!Deaf && !muteshk(shkp)) ? "mentions" : "indicates",
              mhe(shkp), eshk->robbed ? "non-paying" : "rude");
#else
        pline("%s��%s�q�͑匙������%s�D",
              shkname(shkp),
              eshk->robbed ? "�����x����Ȃ�" : "�����",
              (!Deaf && !muteshk(shkp)) ? "������" : "������");
#endif
    } else if (eshk->following) {
        if (strncmp(eshk->customer, plname, PL_NSIZ)) {
            if (!Deaf && !muteshk(shkp))
#if 0 /*JP*/
                verbalize("%s %s!  I was looking for %s.",
                      Hello(shkp), plname, eshk->customer);
#else
                verbalize("%s�I����%s��T���Ă��܂��D",
                      Hello(shkp), eshk->customer);
#endif
            eshk->following = 0;
        } else {
            if (!Deaf && !muteshk(shkp))
#if 0 /*JP*/
                verbalize("%s %s!  Didn't you forget to pay?",
                          Hello(shkp), plname);
#else
                verbalize("%s�I�x������Y��Ă��܂��񂩁H", 
                          Hello(shkp));
#endif
            else
#if 0 /*JP*/
                pline("%s taps you on the %s.",
                      Shknam(shkp), body_part(ARM));
#else
                pline("%s�͂��Ȃ���%s���y�����������D",
                      Shknam(shkp), body_part(ARM));
#endif
        }
    } else if (eshk->billct) {
        register long total = addupbill(shkp) + eshk->debit;

#if 0 /*JP*/
        pline("%s %s that your bill comes to %ld %s.",
              shkname(shkp),
              (!Deaf && !muteshk(shkp)) ? "says" : "indicates",
              total, currency(total));
#else
        pline("%s�͊��肪%ld%s�ɂȂ��%s�D",
              shkname(shkp), total, currency(total),
              (!Deaf && !muteshk(shkp)) ? "������" : "������");
#endif
    } else if (eshk->debit) {
#if 0 /*JP*/
        pline("%s %s that you owe %s %ld %s.",
              shkname(shkp),
              (!Deaf && !muteshk(shkp)) ? "reminds you" : "indicates",
              mhim(shkp), eshk->debit, currency(eshk->debit));
#else
        pline("���Ȃ���%s��%ld%s�̎؂肪���邱�Ƃ��v���������D",
              shkname(shkp), eshk->debit, currency(eshk->debit));
#endif
    } else if (eshk->credit) {
#if 0 /*JP:T*/
        pline("%s encourages you to use your %ld %s of credit.",
              shkname(shkp), eshk->credit, currency(eshk->credit));
#else
        pline("%s��%ld%s�̗a�������g���悤���߂��D",
              shkname(shkp), eshk->credit, currency(eshk->credit));
#endif
    } else if (eshk->robbed) {
#if 0 /*JP*/
        pline("%s %s about a recent robbery.",
              Shknam(shkp),
              (!Deaf && !muteshk(shkp)) ? "complains" : "indicates concern");
#else
        pline("%s�͍ŋ߂̋����ɂ���%s�D",
              Shknam(shkp),
              (!Deaf && !muteshk(shkp)) ? "��s�����ڂ���" : "���O��������");
#endif
    } else if ((shkmoney = money_cnt(shkp->minvent)) < 50L) {
#if 0 /*JP*/
        pline("%s %s that business is bad.",
              shkname(shkp),
              (!Deaf && !muteshk(shkp)) ? "complains" : "indicates");
#else
        pline("%s�͏������|�������ĂȂ���%s�D",
              shkname(shkp),
              (!Deaf && !muteshk(shkp)) ? "��s�����ڂ���" : "������");
#endif
    } else if (shkmoney > 4000) {
#if 0 /*JP*/
        pline("%s %s that business is good.",
              shkname(shkp),
              (!Deaf && !muteshk(shkp)) ? "says" : "indicates");
#else
        pline("%s�͏������|�������Ă����%s�D",
              shkname(shkp),
              (!Deaf && !muteshk(shkp)) ? "������" : "������");
#endif
    } else if (is_izchak(shkp, FALSE)) {
        if (!Deaf && !muteshk(shkp))
            pline(Izchak_speaks[rn2(SIZE(Izchak_speaks))], shkname(shkp));
    } else {
        if (!Deaf && !muteshk(shkp))
/*JP
        pline("%s talks about the problem of shoplifters.", shkname(shkp));
*/
        pline("%s�͖����̖��ɂ��Ęb�����D", shkname(shkp));
    }
}

STATIC_OVL void
kops_gone(silent)
boolean silent;
{
    register int cnt = 0;
    register struct monst *mtmp, *mtmp2;

    for (mtmp = fmon; mtmp; mtmp = mtmp2) {
        mtmp2 = mtmp->nmon;
        if (mtmp->data->mlet == S_KOP) {
            if (canspotmon(mtmp))
                cnt++;
            mongone(mtmp);
        }
    }
    if (cnt && !silent)
#if 0 /*JP*/
        pline_The("Kop%s (disappointed) vanish%s into thin air.",
                  plur(cnt), (cnt == 1) ? "es" : "");
#else
        pline("�x����(�������肵��)�p���������D");
#endif
}

STATIC_OVL long
cost_per_charge(shkp, otmp, altusage)
struct monst *shkp;
struct obj *otmp;
boolean altusage; /* some items have an "alternate" use with different cost */
{
    long tmp = 0L;

    if (!shkp || !inhishop(shkp))
        return 0L; /* insurance */
    tmp = get_cost(otmp, shkp);

    /* The idea is to make the exhaustive use of an unpaid item
     * more expensive than buying it outright.
     */
    if (otmp->otyp == MAGIC_LAMP) { /* 1 */
        /* normal use (ie, as light source) of a magic lamp never
           degrades its value, but not charging anything would make
           identification too easy; charge an amount comparable to
           what is charged for an ordinary lamp (don't bother with
           angry shk surcharge) */
        if (!altusage)
            tmp = (long) objects[OIL_LAMP].oc_cost;
        else
            tmp += tmp / 3L;                 /* djinni is being released */
    } else if (otmp->otyp == MAGIC_MARKER) { /* 70 - 100 */
        /* No way to determine in advance how many charges will be
         * wasted.  So, arbitrarily, one half of the price per use.
         */
        tmp /= 2L;
    } else if (otmp->otyp == BAG_OF_TRICKS /* 1 - 20 */
               || otmp->otyp == HORN_OF_PLENTY) {
        /* altusage: emptying of all the contents at once */
        if (!altusage)
            tmp /= 5L;
    } else if (otmp->otyp == CRYSTAL_BALL               /* 1 - 5 */
               || otmp->otyp == OIL_LAMP                /* 1 - 10 */
               || otmp->otyp == BRASS_LANTERN
               || (otmp->otyp >= MAGIC_FLUTE
                   && otmp->otyp <= DRUM_OF_EARTHQUAKE) /* 5 - 9 */
               || otmp->oclass == WAND_CLASS) {         /* 3 - 11 */
        if (otmp->spe > 1)
            tmp /= 4L;
    } else if (otmp->oclass == SPBOOK_CLASS) {
        tmp -= tmp / 5L;
    } else if (otmp->otyp == CAN_OF_GREASE || otmp->otyp == TINNING_KIT
               || otmp->otyp == EXPENSIVE_CAMERA) {
        tmp /= 10L;
    } else if (otmp->otyp == POT_OIL) {
        tmp /= 5L;
    }
    return tmp;
}

/* Charge the player for partial use of an unpaid object.
 *
 * Note that bill_dummy_object() should be used instead
 * when an object is completely used.
 */
void
check_unpaid_usage(otmp, altusage)
struct obj *otmp;
boolean altusage;
{
    struct monst *shkp;
    const char *fmt, *arg1, *arg2;
    char buf[BUFSZ];
    long tmp;

    if (!otmp->unpaid || !*u.ushops
        || (otmp->spe <= 0 && objects[otmp->otyp].oc_charged))
        return;
    if (!(shkp = shop_keeper(*u.ushops)) || !inhishop(shkp))
        return;
    if ((tmp = cost_per_charge(shkp, otmp, altusage)) == 0L)
        return;

    arg1 = arg2 = "";
    if (otmp->oclass == SPBOOK_CLASS) {
/*JP
        fmt = "%sYou owe%s %ld %s.";
*/
        fmt = "%s%s%ld%s�̎؂肾�D";
/*JP
        Sprintf(buf, "This is no free library, %s!  ", cad(FALSE));
*/
        Strcpy(buf, "�����I�����͐}���ق���Ȃ��I");
        arg1 = rn2(2) ? buf : "";
/*JP
        arg2 = ESHK(shkp)->debit > 0L ? " an additional" : "";
*/
        arg2 = ESHK(shkp)->debit > 0L ? "����Ɍ�����" : "";
    } else if (otmp->otyp == POT_OIL) {
/*JP
        fmt = "%s%sThat will cost you %ld %s (Yendorian Fuel Tax).";
*/
        fmt = "%s%s�l�i��%ld%s(�C�F���_�[�R����)���D";
    } else if (altusage && (otmp->otyp == BAG_OF_TRICKS
                            || otmp->otyp == HORN_OF_PLENTY)) {
/*JP
        fmt = "%s%sEmptying that will cost you %ld %s.";
*/
        fmt = "%s%s����̎g�p����%ld%s���D";
        if (!rn2(3))
/*JP
            arg1 = "Whoa!  ";
*/
            arg1 = "����I";
        if (!rn2(3))
/*JP
            arg1 = "Watch it!  ";
*/
            arg1 = "�C������I";
    } else {
/*JP
        fmt = "%s%sUsage fee, %ld %s.";
*/
        fmt = "%s%s�g�p���́C%ld%s���D";
        if (!rn2(3))
/*JP
            arg1 = "Hey!  ";
*/
            arg1 = "�����I";
        if (!rn2(3))
/*JP
            arg2 = "Ahem.  ";
*/
            arg2 = "�S�z���D";
    }

    if (!Deaf && !muteshk(shkp)) {
        verbalize(fmt, arg1, arg2, tmp, currency(tmp));
        exercise(A_WIS, TRUE); /* you just got info */
    }
    ESHK(shkp)->debit += tmp;
}

/* for using charges of unpaid objects "used in the normal manner" */
void
check_unpaid(otmp)
struct obj *otmp;
{
    check_unpaid_usage(otmp, FALSE); /* normal item use */
}

void
costly_gold(x, y, amount)
register xchar x, y;
register long amount;
{
    register long delta;
    register struct monst *shkp;
    register struct eshk *eshkp;

    if (!costly_spot(x, y))
        return;
    /* shkp now guaranteed to exist by costly_spot() */
    shkp = shop_keeper(*in_rooms(x, y, SHOPBASE));

    eshkp = ESHK(shkp);
    if (eshkp->credit >= amount) {
        if (eshkp->credit > amount)
/*JP
            Your("credit is reduced by %ld %s.", amount, currency(amount));
*/
            Your("�a������%ld%s�������D", amount, currency(amount));
        else
/*JP
            Your("credit is erased.");
*/
            Your("�a�����͒������ɂȂ����D");
        eshkp->credit -= amount;
    } else {
        delta = amount - eshkp->credit;
        if (eshkp->credit)
/*JP
            Your("credit is erased.");
*/
            Your("�a�����͒������ɂȂ����D");
        if (eshkp->debit)
/*JP
            Your("debt increases by %ld %s.", delta, currency(delta));
*/
            Your("�؋���%ld%s�������D", delta, currency(delta));
        else
/*JP
            You("owe %s %ld %s.", shkname(shkp), delta, currency(delta));
*/
            You("%s��%ld%s�̎؂���������D", shkname(shkp), delta, currency(delta));
        eshkp->debit += delta;
        eshkp->loan += delta;
        eshkp->credit = 0L;
    }
}

/* used in domove to block diagonal shop-exit */
/* x,y should always be a door */
boolean
block_door(x, y)
register xchar x, y;
{
    register int roomno = *in_rooms(x, y, SHOPBASE);
    register struct monst *shkp;

    if (roomno < 0 || !IS_SHOP(roomno))
        return FALSE;
    if (!IS_DOOR(levl[x][y].typ))
        return FALSE;
    if (roomno != *u.ushops)
        return FALSE;

    if (!(shkp = shop_keeper((char) roomno)) || !inhishop(shkp))
        return FALSE;

    if (shkp->mx == ESHK(shkp)->shk.x && shkp->my == ESHK(shkp)->shk.y
        /* Actually, the shk should be made to block _any_
         * door, including a door the player digs, if the
         * shk is within a 'jumping' distance.
         */
        && ESHK(shkp)->shd.x == x
        && ESHK(shkp)->shd.y == y
        && shkp->mcanmove && !shkp->msleeping
        && (ESHK(shkp)->debit || ESHK(shkp)->billct || ESHK(shkp)->robbed)) {
#if 0 /*JP*/
        pline("%s%s blocks your way!", shkname(shkp),
              Invis ? " senses your motion and" : "");
#else
        pline("%s��%s���Ȃ��̑O�ɗ����ӂ��������I", shkname(shkp),
              Invis ? "�����������Ƃ�C" : "");
#endif
        return TRUE;
    }
    return FALSE;
}

/* used in domove to block diagonal shop-entry;
   u.ux, u.uy should always be a door */
boolean
block_entry(x, y)
register xchar x, y;
{
    register xchar sx, sy;
    register int roomno;
    register struct monst *shkp;

    if (!(IS_DOOR(levl[u.ux][u.uy].typ)
          && levl[u.ux][u.uy].doormask == D_BROKEN))
        return FALSE;

    roomno = *in_rooms(x, y, SHOPBASE);
    if (roomno < 0 || !IS_SHOP(roomno))
        return FALSE;
    if (!(shkp = shop_keeper((char) roomno)) || !inhishop(shkp))
        return FALSE;

    if (ESHK(shkp)->shd.x != u.ux || ESHK(shkp)->shd.y != u.uy)
        return FALSE;

    sx = ESHK(shkp)->shk.x;
    sy = ESHK(shkp)->shk.y;

    if (shkp->mx == sx && shkp->my == sy && shkp->mcanmove && !shkp->msleeping
        && (x == sx - 1 || x == sx + 1 || y == sy - 1 || y == sy + 1)
        && (Invis || carrying(PICK_AXE) || carrying(DWARVISH_MATTOCK)
            || u.usteed)) {
#if 0 /*JP*/
        pline("%s%s blocks your way!", shkname(shkp),
              Invis ? " senses your motion and" : "");
#else
        pline("%s%s���Ȃ��̑O�ɗ����ӂ��������I", shkname(shkp),
              Invis ? "�����������Ƃ�C" : "");
#endif
        return TRUE;
    }
    return FALSE;
}

/* "your " or "Foobar's " (note the trailing space) */
/* �u���Ȃ��́v���uFoobar�́v(�����ɋ󔒂͕s�v) */
char *
shk_your(buf, obj)
char *buf;
struct obj *obj;
{
    if (!shk_owns(buf, obj) && !mon_owns(buf, obj))
        Strcpy(buf, the_your[carried(obj) ? 1 : 0]);
/*JP
    return strcat(buf, " ");
*/
    return buf;
}

char *
Shk_Your(buf, obj)
char *buf;
struct obj *obj;
{
    (void) shk_your(buf, obj);
#if 0 /*JP*/
    *buf = highc(*buf);
#endif
    return buf;
}

STATIC_OVL char *
shk_owns(buf, obj)
char *buf;
struct obj *obj;
{
    struct monst *shkp;
    xchar x, y;

    if (get_obj_location(obj, &x, &y, 0)
        && (obj->unpaid || (obj->where == OBJ_FLOOR && !obj->no_charge
                            && costly_spot(x, y)))) {
        shkp = shop_keeper(inside_shop(x, y));
#if 0 /*JP*/
        return strcpy(buf, shkp ? s_suffix(shkname(shkp)) : the_your[0]);
#else
        if (shkp) {
            strcpy(buf, shkname(shkp));
            strcat(buf, "��");
        } else {
            strcpy(buf, "");
        }
        return buf;
#endif
    }
    return (char *) 0;
}

STATIC_OVL char *
mon_owns(buf, obj)
char *buf;
struct obj *obj;
{
    if (obj->where == OBJ_MINVENT)
#if 0 /*JP*/
        return strcpy(buf, s_suffix(y_monnam(obj->ocarry)));
#else
        {
            strcpy(buf, mon_nam(obj->ocarry));
            strcat(buf, "��");
            return buf;
        }
#endif
    return (char *) 0;
}

#if 0 /*JP*//*���\�ɌĂт�����Ƃ��̓�l�́B���{��ł́u�����v�ōςނ̂Ŏg��Ȃ�*/
STATIC_OVL const char *
cad(altusage)
boolean altusage; /* used as a verbalized exclamation:  \"Cad! ...\" */
{
    const char *res = 0;

    switch (is_demon(youmonst.data) ? 3 : poly_gender()) {
    case 0:
        res = "cad";
        break;
    case 1:
        res = "minx";
        break;
    case 2:
        res = "beast";
        break;
    case 3:
        res = "fiend";
        break;
    default:
        impossible("cad: unknown gender");
        res = "thing";
        break;
    }
    if (altusage) {
        char *cadbuf = mon_nam(&youmonst); /* snag an output buffer */

        /* alternate usage adds a leading double quote and trailing
           exclamation point plus sentence separating spaces */
        Sprintf(cadbuf, "\"%s!  ", res);
        cadbuf[1] = highc(cadbuf[1]);
        res = cadbuf;
    }
    return res;
}
#endif /*JP*/

#ifdef __SASC
void
sasc_bug(struct obj *op, unsigned x)
{
    op->unpaid = x;
}
#endif

/*shk.c*/
