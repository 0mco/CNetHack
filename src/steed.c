/* NetHack 3.6	steed.c	$NHDT-Date: 1445906867 2015/10/27 00:47:47 $  $NHDT-Branch: master $:$NHDT-Revision: 1.47 $ */
/* Copyright (c) Kevin Hugo, 1998-1999. */
/* NetHack may be freely redistributed.  See license for details. */

/* JNetHack Copyright */
/* (c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994-2000  */
/* For 3.4-, Copyright (c) SHIRAKATA Kentaro, 2002-2018            */
/* JNetHack may be freely redistributed.  See license for details. */

#include "hack.h"

/* Monsters that might be ridden */
static NEARDATA const char steeds[] = { S_QUADRUPED, S_UNICORN, S_ANGEL,
                                        S_CENTAUR,   S_DRAGON,  S_JABBERWOCK,
                                        '\0' };

STATIC_DCL boolean FDECL(landing_spot, (coord *, int, int));
STATIC_DCL void FDECL(maybewakesteed, (struct monst *));

/* caller has decided that hero can't reach something while mounted */
void
rider_cant_reach()
{
/*JP
    You("aren't skilled enough to reach from %s.", y_monnam(u.usteed));
*/
    You("�܂��\���ɋZ�ʂ�ς�ł��Ȃ��̂ŁC%s����͂��Ȃ��D", y_monnam(u.usteed));
}

/*** Putting the saddle on ***/

/* Can this monster wear a saddle? */
boolean
can_saddle(mtmp)
struct monst *mtmp;
{
    struct permonst *ptr = mtmp->data;

    return (index(steeds, ptr->mlet) && (ptr->msize >= MZ_MEDIUM)
            && (!humanoid(ptr) || ptr->mlet == S_CENTAUR) && !amorphous(ptr)
            && !noncorporeal(ptr) && !is_whirly(ptr) && !unsolid(ptr));
}

int
use_saddle(otmp)
struct obj *otmp;
{
    struct monst *mtmp;
    struct permonst *ptr;
    int chance;
    const char *s;

    if (!u_handsy())
        return 0;

    /* Select an animal */
    if (u.uswallow || Underwater || !getdir((char *) 0)) {
        pline1(Never_mind);
        return 0;
    }
    if (!u.dx && !u.dy) {
/*JP
        pline("Saddle yourself?  Very funny...");
*/
        pline("�������g�ɈƁH�������낢�D�D�D");
        return 0;
    }
    if (!isok(u.ux + u.dx, u.uy + u.dy)
        || !(mtmp = m_at(u.ux + u.dx, u.uy + u.dy)) || !canspotmon(mtmp)) {
/*JP
        pline("I see nobody there.");
*/
        pline("�����ɂ͒N�����Ȃ��悤�Ɍ�����D");
        return 1;
    }

    /* Is this a valid monster? */
    if (mtmp->misc_worn_check & W_SADDLE || which_armor(mtmp, W_SADDLE)) {
/*JP
        pline("%s doesn't need another one.", Monnam(mtmp));
*/
        pline("%s�͂����Ƃ��������Ă���D", Monnam(mtmp));
        return 1;
    }
    ptr = mtmp->data;
    if (touch_petrifies(ptr) && !uarmg && !Stone_resistance) {
        char kbuf[BUFSZ];

/*JP
        You("touch %s.", mon_nam(mtmp));
*/
        You("%s�ɐG�ꂽ�D", mon_nam(mtmp));
        if (!(poly_when_stoned(youmonst.data) && polymon(PM_STONE_GOLEM))) {
/*JP
            Sprintf(kbuf, "attempting to saddle %s", an(mtmp->data->mname));
*/
            Sprintf(kbuf, "%s�ɈƂ������悤�Ƃ���", mtmp->data->mname);
            instapetrify(kbuf);
        }
    }
    if (ptr == &mons[PM_INCUBUS] || ptr == &mons[PM_SUCCUBUS]) {
/*JP
        pline("Shame on you!");
*/
        pline("�p��m��I");
        exercise(A_WIS, FALSE);
        return 1;
    }
    if (mtmp->isminion || mtmp->isshk || mtmp->ispriest || mtmp->isgd
        || mtmp->iswiz) {
/*JP
        pline("I think %s would mind.", mon_nam(mtmp));
*/
        pline("%s�͌������Ă���悤���D", mon_nam(mtmp));
        return 1;
    }
    if (!can_saddle(mtmp)) {
/*JP
        You_cant("saddle such a creature.");
*/
        You("���̐������ɈƂ͂Ƃ�����Ȃ��D");
        return 1;
    }

    /* Calculate your chance */
    chance = ACURR(A_DEX) + ACURR(A_CHA) / 2 + 2 * mtmp->mtame;
    chance += u.ulevel * (mtmp->mtame ? 20 : 5);
    if (!mtmp->mtame)
        chance -= 10 * mtmp->m_lev;
    if (Role_if(PM_KNIGHT))
        chance += 20;
    switch (P_SKILL(P_RIDING)) {
    case P_ISRESTRICTED:
    case P_UNSKILLED:
    default:
        chance -= 20;
        break;
    case P_BASIC:
        break;
    case P_SKILLED:
        chance += 15;
        break;
    case P_EXPERT:
        chance += 30;
        break;
    }
    if (Confusion || Fumbling || Glib)
        chance -= 20;
    else if (uarmg && (s = OBJ_DESCR(objects[uarmg->otyp])) != (char *) 0
/*JP
             && !strncmp(s, "riding ", 7))
*/
             && !strncmp(s, "��n�p��", 8))
        /* Bonus for wearing "riding" (but not fumbling) gloves */
        chance += 10;
    else if (uarmf && (s = OBJ_DESCR(objects[uarmf->otyp])) != (char *) 0
/*JP
             && !strncmp(s, "riding ", 7))
*/
             && !strncmp(s, "��n�p��", 8))
        /* ... or for "riding boots" */
        chance += 10;
    if (otmp->cursed)
        chance -= 50;

    /* [intended] steed becomes alert if possible */
    maybewakesteed(mtmp);

    /* Make the attempt */
    if (rn2(100) < chance) {
/*JP
        You("put the saddle on %s.", mon_nam(mtmp));
*/
        You("�Ƃ�%s�Ɏ������D", mon_nam(mtmp));
        if (otmp->owornmask)
            remove_worn_item(otmp, FALSE);
        freeinv(otmp);
        put_saddle_on_mon(otmp, mtmp);
    } else
/*JP
        pline("%s resists!", Monnam(mtmp));
*/
        pline("%s�͋��ۂ����I", Monnam(mtmp));
    return 1;
}

void
put_saddle_on_mon(saddle, mtmp)
struct obj *saddle;
struct monst *mtmp;
{
    if (!can_saddle(mtmp) || which_armor(mtmp, W_SADDLE))
        return;
    if (mpickobj(mtmp, saddle))
        panic("merged saddle?");
    mtmp->misc_worn_check |= W_SADDLE;
    saddle->owornmask = W_SADDLE;
    saddle->leashmon = mtmp->m_id;
    update_mon_intrinsics(mtmp, saddle, TRUE, FALSE);
}

/*** Riding the monster ***/

/* Can we ride this monster?  Caller should also check can_saddle() */
boolean
can_ride(mtmp)
struct monst *mtmp;
{
    return (mtmp->mtame && humanoid(youmonst.data)
            && !verysmall(youmonst.data) && !bigmonst(youmonst.data)
            && (!Underwater || is_swimmer(mtmp->data)));
}

int
doride()
{
    boolean forcemount = FALSE;

    if (u.usteed) {
        dismount_steed(DISMOUNT_BYCHOICE);
    } else if (getdir((char *) 0) && isok(u.ux + u.dx, u.uy + u.dy)) {
/*JP
        if (wizard && yn("Force the mount to succeed?") == 'y')
*/
        if (wizard && yn("��������������܂����H") == 'y')
            forcemount = TRUE;
        return (mount_steed(m_at(u.ux + u.dx, u.uy + u.dy), forcemount));
    } else {
        return 0;
    }
    return 1;
}

/* Start riding, with the given monster */
boolean
mount_steed(mtmp, force)
struct monst *mtmp; /* The animal */
boolean force;      /* Quietly force this animal */
{
    struct obj *otmp;
    char buf[BUFSZ];
    struct permonst *ptr;

    /* Sanity checks */
    if (u.usteed) {
/*JP
        You("are already riding %s.", mon_nam(u.usteed));
*/
        You("����%s�ɏ���Ă���D", mon_nam(u.usteed));
        return (FALSE);
    }

    /* Is the player in the right form? */
    if (Hallucination && !force) {
#if 0 /*JP*/
        pline("Maybe you should find a designated driver.");
#else
        pline("�����炭���Ȃ��͎w��h���C�o�[��T���ׂ����낤�D");
#endif
        return (FALSE);
    }
    /* While riding Wounded_legs refers to the steed's,
     * not the hero's legs.
     * That opens up a potential abuse where the player
     * can mount a steed, then dismount immediately to
     * heal leg damage, because leg damage is always
     * healed upon dismount (Wounded_legs context switch).
     * By preventing a hero with Wounded_legs from
     * mounting a steed, the potential for abuse is
     * reduced.  However, dismounting still immediately
     * heals the steed's wounded legs.  [In 3.4.3 and
     * earlier, that unintentionally made the hero's
     * temporary 1 point Dex loss become permanent.]
     */
    if (Wounded_legs) {
/*JP
        Your("%s are in no shape for riding.", makeplural(body_part(LEG)));
*/
        pline("%s�����䂵�Ă���̂ŏ��Ȃ��D", makeplural(body_part(LEG)));
        if (force && wizard && yn("Heal your legs?") == 'y')
            HWounded_legs = EWounded_legs = 0;
        else
            return (FALSE);
    }

    if (Upolyd && (!humanoid(youmonst.data) || verysmall(youmonst.data)
                   || bigmonst(youmonst.data) || slithy(youmonst.data))) {
/*JP
        You("won't fit on a saddle.");
*/
        You("�Ƃɍ���Ȃ��D");
        return (FALSE);
    }
    if (!force && (near_capacity() > SLT_ENCUMBER)) {
/*JP
        You_cant("do that while carrying so much stuff.");
*/
        You("��R�������������Ă���o���Ȃ��D");
        return (FALSE);
    }

    /* Can the player reach and see the monster? */
    if (!mtmp || (!force && ((Blind && !Blind_telepat) || mtmp->mundetected
                             || mtmp->m_ap_type == M_AP_FURNITURE
                             || mtmp->m_ap_type == M_AP_OBJECT))) {
/*JP
        pline("I see nobody there.");
*/
        pline("�����ɂ͉��������Ȃ��D");
        return (FALSE);
    }
    if (u.uswallow || u.ustuck || u.utrap || Punished
        || !test_move(u.ux, u.uy, mtmp->mx - u.ux, mtmp->my - u.uy,
                      TEST_MOVE)) {
        if (Punished || !(u.uswallow || u.ustuck || u.utrap))
/*JP
            You("are unable to swing your %s over.", body_part(LEG));
*/
            You("�Ƃ��܂������Ƃ��ł��Ȃ��D");
        else
/*JP
            You("are stuck here for now.");
*/
            You("�͂܂��Ă���̂ŏo���Ȃ��D");
        return (FALSE);
    }

    /* Is this a valid monster? */
    otmp = which_armor(mtmp, W_SADDLE);
    if (!otmp) {
/*JP
        pline("%s is not saddled.", Monnam(mtmp));
*/
        pline("%s�ɂ͈Ƃ��������Ă��Ȃ��D", Monnam(mtmp));
        return (FALSE);
    }
    ptr = mtmp->data;
    if (touch_petrifies(ptr) && !Stone_resistance) {
        char kbuf[BUFSZ];

/*JP
        You("touch %s.", mon_nam(mtmp));
*/
        You("%s�ɐG�ꂽ�D", mon_nam(mtmp));
/*JP
        Sprintf(kbuf, "attempting to ride %s", an(mtmp->data->mname));
*/
        Sprintf(kbuf, "%s�ɏ�낤�Ƃ���", a_monnam(mtmp));
        instapetrify(kbuf);
    }
    if (!mtmp->mtame || mtmp->isminion) {
/*JP
        pline("I think %s would mind.", mon_nam(mtmp));
*/
        pline("%s�͌������Ă���悤���D", mon_nam(mtmp));
        return (FALSE);
    }
    if (mtmp->mtrapped) {
        struct trap *t = t_at(mtmp->mx, mtmp->my);

#if 0 /*JP*/
        You_cant("mount %s while %s's trapped in %s.", mon_nam(mtmp),
                 mhe(mtmp), an(defsyms[trap_to_defsym(t->ttyp)].explanation));
#else
        You("%s�ɕ߂܂��Ă���%s�ɂ͏��Ȃ��D",
            defsyms[trap_to_defsym(t->ttyp)].explanation, mon_nam(mtmp));
#endif
        return (FALSE);
    }

    if (!force && !Role_if(PM_KNIGHT) && !(--mtmp->mtame)) {
        /* no longer tame */
        newsym(mtmp->mx, mtmp->my);
#if 0 /*JP*/
        pline("%s resists%s!", Monnam(mtmp),
              mtmp->mleashed ? " and its leash comes off" : "");
#else
        pline("%s�͋���%s�I", Monnam(mtmp),
              mtmp->mleashed ? "���āC�R���͂�����" : "����");
#endif
        if (mtmp->mleashed)
            m_unleash(mtmp, FALSE);
        return (FALSE);
    }
    if (!force && Underwater && !is_swimmer(ptr)) {
#if 0 /*JP*/
        You_cant("ride that creature while under %s.",
                 hliquid("water"));
#else /*�Ƃ肠����������*/
        You("�����ŏ�邱�Ƃ͂ł��Ȃ��D");
#endif
        return (FALSE);
    }
    if (!can_saddle(mtmp) || !can_ride(mtmp)) {
/*JP
        You_cant("ride such a creature.");
*/
        You("���̐������ɏ�邱�Ƃ͂ł��Ȃ��D");
        return FALSE;
    }

    /* Is the player impaired? */
    if (!force && !is_floater(ptr) && !is_flyer(ptr) && Levitation
        && !Lev_at_will) {
/*JP
        You("cannot reach %s.", mon_nam(mtmp));
*/
        You("%s�ɓ͂��Ȃ��D", mon_nam(mtmp));
        return (FALSE);
    }
    if (!force && uarm && is_metallic(uarm) && greatest_erosion(uarm)) {
#if 0 /*JP*/
        Your("%s armor is too stiff to be able to mount %s.",
             uarm->oeroded ? "rusty" : "corroded", mon_nam(mtmp));
#else
        Your("%s�Z�̓M�V�M�V�����Ă���%s�ɏ��Ȃ��D",
             uarm->oeroded ? "�K�т�" : "���H����", mon_nam(mtmp));
#endif
        return (FALSE);
    }
    if (!force
        && (Confusion || Fumbling || Glib || Wounded_legs || otmp->cursed
            || (u.ulevel + mtmp->mtame < rnd(MAXULEV / 2 + 5)))) {
        if (Levitation) {
/*JP
            pline("%s slips away from you.", Monnam(mtmp));
*/
            pline("%s�͂��Ȃ�����͂Ȃ�Ă������D", Monnam(mtmp));
            return FALSE;
        }
/*JP
        You("slip while trying to get on %s.", mon_nam(mtmp));
*/
        You("%s�ɏ�낤�Ƃ��Ă��ׂ����D", mon_nam(mtmp));

/*JP
        Sprintf(buf, "slipped while mounting %s",
*/
        Sprintf(buf, "%s�ɏ�낤�Ƃ��Ă��ׂ藎����",
                /* "a saddled mumak" or "a saddled pony called Dobbin" */
                x_monnam(mtmp, ARTICLE_A, (char *) 0,
                         SUPPRESS_IT | SUPPRESS_INVISIBLE
                             | SUPPRESS_HALLUCINATION,
                         TRUE));
#if 0 /*JP*/
        losehp(Maybe_Half_Phys(rn1(5, 10)), buf, NO_KILLER_PREFIX);
#else
        losehp(Maybe_Half_Phys(rn1(5, 10)), buf, KILLED_BY);
#endif
        return (FALSE);
    }

    /* Success */
    maybewakesteed(mtmp);
    if (!force) {
        if (Levitation && !is_floater(ptr) && !is_flyer(ptr))
            /* Must have Lev_at_will at this point */
/*JP
            pline("%s magically floats up!", Monnam(mtmp));
*/
            pline("%s�͖��@�̗͂ŕ������I", Monnam(mtmp));
/*JP
        You("mount %s.", mon_nam(mtmp));
*/
        You("%s�ɏ�����D", mon_nam(mtmp));
    }
    /* setuwep handles polearms differently when you're mounted */
    if (uwep && is_pole(uwep))
        unweapon = FALSE;
    u.usteed = mtmp;
    remove_monster(mtmp->mx, mtmp->my);
    teleds(mtmp->mx, mtmp->my, TRUE);
    context.botl = TRUE;
    return TRUE;
}

/* You and your steed have moved */
void
exercise_steed()
{
    if (!u.usteed)
        return;

    /* It takes many turns of riding to exercise skill */
    if (u.urideturns++ >= 100) {
        u.urideturns = 0;
        use_skill(P_RIDING, 1);
    }
    return;
}

/* The player kicks or whips the steed */
void
kick_steed()
{
#if 0 /*JP*/
    char He[4];
#else
    /* role.c Gender.he �̒l���R�s�[����� */
    char He[16];
#endif
    if (!u.usteed)
        return;

    /* [ALI] Various effects of kicking sleeping/paralyzed steeds */
    if (u.usteed->msleeping || !u.usteed->mcanmove) {
        /* We assume a message has just been output of the form
         * "You kick <steed>."
         */
        Strcpy(He, mhe(u.usteed));
        *He = highc(*He);
        if ((u.usteed->mcanmove || u.usteed->mfrozen) && !rn2(2)) {
            if (u.usteed->mcanmove)
                u.usteed->msleeping = 0;
            else if (u.usteed->mfrozen > 2)
                u.usteed->mfrozen -= 2;
            else {
                u.usteed->mfrozen = 0;
                u.usteed->mcanmove = 1;
            }
            if (u.usteed->msleeping || !u.usteed->mcanmove)
#if 0 /*JP*/
                pline("%s stirs.", He);
#else
                pline("%s�͐g���났�����D", He);
#endif
            else
#if 0 /*JP*/
                pline("%s rouses %sself!", He, mhim(u.usteed));
#else
                pline("%s�͕��N�����I", He);
#endif
        } else
#if 0 /*JP*/
            pline("%s does not respond.", He);
#else
            pline("%s�͔������Ȃ��D", He);
#endif
        return;
    }

    /* Make the steed less tame and check if it resists */
    if (u.usteed->mtame)
        u.usteed->mtame--;
    if (!u.usteed->mtame && u.usteed->mleashed)
        m_unleash(u.usteed, TRUE);
    if (!u.usteed->mtame
        || (u.ulevel + u.usteed->mtame < rnd(MAXULEV / 2 + 5))) {
        newsym(u.usteed->mx, u.usteed->my);
        dismount_steed(DISMOUNT_THROWN);
        return;
    }

/*JP
    pline("%s gallops!", Monnam(u.usteed));
*/
    pline("%s�͑����ɂȂ����I", Monnam(u.usteed));
    u.ugallop += rn1(20, 30);
    return;
}

/*
 * Try to find a dismount point adjacent to the steed's location.
 * If all else fails, try enexto().  Use enexto() as a last resort because
 * enexto() chooses its point randomly, possibly even outside the
 * room's walls, which is not what we want.
 * Adapted from mail daemon code.
 */
STATIC_OVL boolean
landing_spot(spot, reason, forceit)
coord *spot; /* landing position (we fill it in) */
int reason;
int forceit;
{
    int i = 0, x, y, distance, min_distance = -1;
    boolean found = FALSE;
    struct trap *t;

    /* avoid known traps (i == 0) and boulders, but allow them as a backup */
    if (reason != DISMOUNT_BYCHOICE || Stunned || Confusion || Fumbling)
        i = 1;
    for (; !found && i < 2; ++i) {
        for (x = u.ux - 1; x <= u.ux + 1; x++)
            for (y = u.uy - 1; y <= u.uy + 1; y++) {
                if (!isok(x, y) || (x == u.ux && y == u.uy))
                    continue;

                if (accessible(x, y) && !MON_AT(x, y)) {
                    distance = distu(x, y);
                    if (min_distance < 0 || distance < min_distance
                        || (distance == min_distance && rn2(2))) {
                        if (i > 0 || (((t = t_at(x, y)) == 0 || !t->tseen)
                                      && (!sobj_at(BOULDER, x, y)
                                          || throws_rocks(youmonst.data)))) {
                            spot->x = x;
                            spot->y = y;
                            min_distance = distance;
                            found = TRUE;
                        }
                    }
                }
            }
    }

    /* If we didn't find a good spot and forceit is on, try enexto(). */
    if (forceit && min_distance < 0
        && !enexto(spot, u.ux, u.uy, youmonst.data))
        return FALSE;

    return found;
}

/* Stop riding the current steed */
void
dismount_steed(reason)
int reason; /* Player was thrown off etc. */
{
    struct monst *mtmp;
    struct obj *otmp;
    coord cc;
/*JP
    const char *verb = "fall";
*/
    const char *verb = "������";
    boolean repair_leg_damage = (Wounded_legs != 0L);
    unsigned save_utrap = u.utrap;
    boolean have_spot = landing_spot(&cc, reason, 0);

    mtmp = u.usteed; /* make a copy of steed pointer */
    /* Sanity check */
    if (!mtmp) /* Just return silently */
        return;

    /* Check the reason for dismounting */
    otmp = which_armor(mtmp, W_SADDLE);
    switch (reason) {
    case DISMOUNT_THROWN:
/*JP
        verb = "are thrown";
*/
        verb = "�ӂ藎���ꂽ";
        /*FALLTHRU*/
    case DISMOUNT_FELL:
/*JP
        You("%s off of %s!", verb, mon_nam(mtmp));
*/
        You("%s����%s�I", mon_nam(mtmp), verb);
        if (!have_spot)
            have_spot = landing_spot(&cc, reason, 1);
/*JP
        losehp(Maybe_Half_Phys(rn1(10, 10)), "riding accident", KILLED_BY_AN);
*/
        losehp(Maybe_Half_Phys(rn1(10, 10)), "�R�掖�̂�", KILLED_BY_AN);
        set_wounded_legs(BOTH_SIDES, (int) HWounded_legs + rn1(5, 5));
        repair_leg_damage = FALSE;
        break;
    case DISMOUNT_POLY:
/*JP
        You("can no longer ride %s.", mon_nam(u.usteed));
*/
        You("%s�ɏ���Ă��Ȃ��D", mon_nam(u.usteed));
        if (!have_spot)
            have_spot = landing_spot(&cc, reason, 1);
        break;
    case DISMOUNT_ENGULFED:
        /* caller displays message */
        break;
    case DISMOUNT_BONES:
        /* hero has just died... */
        break;
    case DISMOUNT_GENERIC:
        /* no messages, just make it so */
        break;
    case DISMOUNT_BYCHOICE:
    default:
        if (otmp && otmp->cursed) {
#if 0 /*JP*/
            You("can't.  The saddle %s cursed.",
                otmp->bknown ? "is" : "seems to be");
#else
            You("�~����Ȃ��D�Ƃ͎���Ă���%s�D",
                otmp->bknown ? "" : "�悤��");
#endif
            otmp->bknown = TRUE;
            return;
        }
        if (!have_spot) {
/*JP
            You("can't. There isn't anywhere for you to stand.");
*/
            pline("���Ȃ��̗��ꏊ���Ȃ��̂ō~����Ȃ��D");
            return;
        }
        if (!has_mname(mtmp)) {
/*JP
            pline("You've been through the dungeon on %s with no name.",
*/
            pline("���Ȃ��͖��O�̂Ȃ�%s�Ƌ��ɖ��{���ɂ���D",
                  an(mtmp->data->mname));
            if (Hallucination)
/*JP
                pline("It felt good to get out of the rain.");
*/
                pline("�J���~��Ȃ��̂Ƃ����̂͂����C���������D");
        } else
/*JP
            You("dismount %s.", mon_nam(mtmp));
*/
            You("%s����~�肽�D", mon_nam(mtmp));
    }
    /* While riding, Wounded_legs refers to the steed's legs;
       after dismounting, it reverts to the hero's legs. */
    if (repair_leg_damage) {
        /* [TODO: make heal_legs() take a parameter to handle this] */
        in_steed_dismounting = TRUE;
        heal_legs();
        in_steed_dismounting = FALSE;
    }

    /* Release the steed and saddle */
    u.usteed = 0;
    u.ugallop = 0L;

    /* Set player and steed's position.  Try moving the player first
       unless we're in the midst of creating a bones file. */
    if (reason == DISMOUNT_BONES) {
        /* move the steed to an adjacent square */
        if (enexto(&cc, u.ux, u.uy, mtmp->data))
            rloc_to(mtmp, cc.x, cc.y);
        else /* evidently no room nearby; move steed elsewhere */
            (void) rloc(mtmp, FALSE);
        return;
    }
    if (mtmp->mhp > 0) {
        place_monster(mtmp, u.ux, u.uy);
        if (!u.uswallow && !u.ustuck && have_spot) {
            struct permonst *mdat = mtmp->data;

            /* The steed may drop into water/lava */
            if (!is_flyer(mdat) && !is_floater(mdat) && !is_clinger(mdat)) {
                if (is_pool(u.ux, u.uy)) {
                    if (!Underwater)
/*JP
                        pline("%s falls into the %s!", Monnam(mtmp),
*/
                        pline("%s��%s�ɗ������I", Monnam(mtmp),
                              surface(u.ux, u.uy));
                    if (!is_swimmer(mdat) && !amphibious(mdat)) {
                        killed(mtmp);
                        adjalign(-1);
                    }
                } else if (is_lava(u.ux, u.uy)) {
#if 0 /*JP*/
                    pline("%s is pulled into the %s!", Monnam(mtmp),
                          hliquid("lava"));
#else
                    pline("%s��%s�̒��ɂЂ��ς�ꂽ�I", Monnam(mtmp),
                          hliquid("�n��"));
#endif
                    if (!likes_lava(mdat)) {
                        killed(mtmp);
                        adjalign(-1);
                    }
                }
            }
            /* Steed dismounting consists of two steps: being moved to another
             * square, and descending to the floor.  We have functions to do
             * each of these activities, but they're normally called
             * individually and include an attempt to look at or pick up the
             * objects on the floor:
             * teleds() --> spoteffects() --> pickup()
             * float_down() --> pickup()
             * We use this kludge to make sure there is only one such attempt.
             *
             * Clearly this is not the best way to do it.  A full fix would
             * involve having these functions not call pickup() at all,
             * instead
             * calling them first and calling pickup() afterwards.  But it
             * would take a lot of work to keep this change from having any
             * unforeseen side effects (for instance, you would no longer be
             * able to walk onto a square with a hole, and autopickup before
             * falling into the hole).
             */
            /* [ALI] No need to move the player if the steed died. */
            if (mtmp->mhp > 0) {
                /* Keep steed here, move the player to cc;
                 * teleds() clears u.utrap
                 */
                in_steed_dismounting = TRUE;
                teleds(cc.x, cc.y, TRUE);
                in_steed_dismounting = FALSE;

                /* Put your steed in your trap */
                if (save_utrap)
                    (void) mintrap(mtmp);
            }
            /* Couldn't... try placing the steed */
        } else if (enexto(&cc, u.ux, u.uy, mtmp->data)) {
            /* Keep player here, move the steed to cc */
            rloc_to(mtmp, cc.x, cc.y);
            /* Player stays put */
            /* Otherwise, kill the steed */
        } else {
            killed(mtmp);
            adjalign(-1);
        }
    }

    /* Return the player to the floor */
    if (reason != DISMOUNT_ENGULFED) {
        in_steed_dismounting = TRUE;
        (void) float_down(0L, W_SADDLE);
        in_steed_dismounting = FALSE;
        context.botl = TRUE;
        (void) encumber_msg();
        vision_full_recalc = 1;
    } else
        context.botl = TRUE;
    /* polearms behave differently when not mounted */
    if (uwep && is_pole(uwep))
        unweapon = TRUE;
    return;
}

/* when attempting to saddle or mount a sleeping steed, try to wake it up
   (for the saddling case, it won't be u.usteed yet) */
STATIC_OVL void
maybewakesteed(steed)
struct monst *steed;
{
    int frozen = (int) steed->mfrozen;
    boolean wasimmobile = steed->msleeping || !steed->mcanmove;

    steed->msleeping = 0;
    if (frozen) {
        frozen = (frozen + 1) / 2; /* half */
        /* might break out of timed sleep or paralysis */
        if (!rn2(frozen)) {
            steed->mfrozen = 0;
            steed->mcanmove = 1;
        } else {
            /* didn't awake, but remaining duration is halved */
            steed->mfrozen = frozen;
        }
    }
    if (wasimmobile && !steed->msleeping && steed->mcanmove)
/*JP
        pline("%s wakes up.", Monnam(steed));
*/
        pline("%s�͋N�����D", Monnam(steed));
    /* regardless of waking, terminate any meal in progress */
    finish_meating(steed);
}

/* decide whether hero's steed is able to move;
   doesn't check for holding traps--those affect the hero directly */
boolean
stucksteed(checkfeeding)
boolean checkfeeding;
{
    struct monst *steed = u.usteed;

    if (steed) {
        /* check whether steed can move */
        if (steed->msleeping || !steed->mcanmove) {
/*JP
            pline("%s won't move!", upstart(y_monnam(steed)));
*/
            pline("%s�͓����Ȃ��I", y_monnam(steed));
            return TRUE;
        }
        /* optionally check whether steed is in the midst of a meal */
        if (checkfeeding && steed->meating) {
/*JP
            pline("%s is still eating.", upstart(y_monnam(steed)));
*/
            pline("%s�͂܂��H�ׂĂ���D", y_monnam(steed));
            return TRUE;
        }
    }
    return FALSE;
}

void
place_monster(mon, x, y)
struct monst *mon;
int x, y;
{
    if (mon == u.usteed
        /* special case is for convoluted vault guard handling */
        || (DEADMONSTER(mon) && !(mon->isgd && x == 0 && y == 0))) {
        impossible("placing %s onto map?",
                   (mon == u.usteed) ? "steed" : "defunct monster");
        return;
    }
    mon->mx = x, mon->my = y;
    level.monsters[x][y] = mon;
}

/*steed.c*/
