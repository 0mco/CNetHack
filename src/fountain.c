/* NetHack 3.6	fountain.c	$NHDT-Date: 1455402364 2016/02/13 22:26:04 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.56 $ */
/*      Copyright Scott R. Turner, srt@ucla, 10/27/86 */
/* NetHack may be freely redistributed.  See license for details. */

/* JNetHack Copyright */
/* (c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994-2000  */
/* For 3.4-, Copyright (c) SHIRAKATA Kentaro, 2002-2018            */
/* JNetHack may be freely redistributed.  See license for details. */

/* Code for drinking from fountains. */

#include "hack.h"

STATIC_DCL void NDECL(dowatersnakes);
STATIC_DCL void NDECL(dowaterdemon);
STATIC_DCL void NDECL(dowaternymph);
STATIC_PTR void FDECL(gush, (int, int, genericptr_t));
STATIC_DCL void NDECL(dofindgem);

/* used when trying to dip in or drink from fountain or sink or pool while
   levitating above it, or when trying to move downwards in that state */
void
floating_above(what)
const char *what;
{
/*JP
    const char *umsg = "are floating high above the %s.";
*/
    const char *umsg = "%s�̗y������ɕ����Ă���D";

    if (u.utrap && (u.utraptype == TT_INFLOOR || u.utraptype == TT_LAVA)) {
        /* when stuck in floor (not possible at fountain or sink location,
           so must be attempting to move down), override the usual message */
/*JP
        umsg = "are trapped in the %s.";
*/
        umsg = "%s�ɂ��܂��Ă���D";
        what = surface(u.ux, u.uy); /* probably redundant */
    }
    You(umsg, what);
}

/* Fountain of snakes! */
STATIC_OVL void
dowatersnakes()
{
    register int num = rn1(5, 2);
    struct monst *mtmp;

    if (!(mvitals[PM_WATER_MOCCASIN].mvflags & G_GONE)) {
        if (!Blind)
#if 0 /*JP*/
            pline("An endless stream of %s pours forth!",
                  Hallucination ? makeplural(rndmonnam(NULL)) : "snakes");
#else
            pline("%s���ǂǂ��Ɨ���o�Ă����I",
                  Hallucination ? rndmonnam(NULL) : "��");
#endif
        else
/*JP
            You_hear("%s hissing!", something);
*/
            You_hear("�V�[�b�Ƃ������𕷂����I");
        while (num-- > 0)
            if ((mtmp = makemon(&mons[PM_WATER_MOCCASIN], u.ux, u.uy,
                                NO_MM_FLAGS)) != 0
                && t_at(mtmp->mx, mtmp->my))
                (void) mintrap(mtmp);
    } else
/*JP
        pline_The("fountain bubbles furiously for a moment, then calms.");
*/
        pline("��͓ˑR�������A�����C�₪�ĐÂ��ɂȂ����D");
}

/* Water demon */
STATIC_OVL void
dowaterdemon()
{
    struct monst *mtmp;

    if (!(mvitals[PM_WATER_DEMON].mvflags & G_GONE)) {
        if ((mtmp = makemon(&mons[PM_WATER_DEMON], u.ux, u.uy,
                            NO_MM_FLAGS)) != 0) {
            if (!Blind)
/*JP
                You("unleash %s!", a_monnam(mtmp));
*/
                You("%s�������������I", a_monnam(mtmp));
            else
/*JP
                You_feel("the presence of evil.");
*/
                You_feel("�׈��ȑ��݂��������I");

            /* Give those on low levels a (slightly) better chance of survival
             */
            if (rnd(100) > (80 + level_difficulty())) {
#if 0 /*JP*/
                pline("Grateful for %s release, %s grants you a wish!",
                      mhis(mtmp), mhe(mtmp));
#else
                pline("%s�͉�����ƂĂ����ӂ��C�̂��݂����Ȃ��Ă����悤���I",
                      mhe(mtmp));
#endif
                /* give a wish and discard the monster (mtmp set to null) */
                mongrantswish(&mtmp);
            } else if (t_at(mtmp->mx, mtmp->my))
                (void) mintrap(mtmp);
        }
    } else
/*JP
        pline_The("fountain bubbles furiously for a moment, then calms.");
*/
        pline("��͓ˑR�������A�����C�₪�ĐÂ��ɂȂ����D");
}

/* Water Nymph */
STATIC_OVL void
dowaternymph()
{
    register struct monst *mtmp;

    if (!(mvitals[PM_WATER_NYMPH].mvflags & G_GONE)
        && (mtmp = makemon(&mons[PM_WATER_NYMPH], u.ux, u.uy,
                           NO_MM_FLAGS)) != 0) {
        if (!Blind)
/*JP
            You("attract %s!", a_monnam(mtmp));
*/
            pline("%s������ꂽ�I", a_monnam(mtmp));
        else
/*JP
            You_hear("a seductive voice.");
*/
            You_hear("���f�I�Ȑ��𕷂����D");
        mtmp->msleeping = 0;
        if (t_at(mtmp->mx, mtmp->my))
            (void) mintrap(mtmp);
    } else if (!Blind)
/*JP
        pline("A large bubble rises to the surface and pops.");
*/
        pline("�傫�ȖA�������o�Ă͂������D");
    else
/*JP
        You_hear("a loud pop.");
*/
        You_hear("�傫�Ȃ��̂��͂����鉹�𕷂����D");
}

/* Gushing forth along LOS from (u.ux, u.uy) */
void
dogushforth(drinking)
int drinking;
{
    int madepool = 0;

    do_clear_area(u.ux, u.uy, 7, gush, (genericptr_t) &madepool);
    if (!madepool) {
        if (drinking)
/*JP
            Your("thirst is quenched.");
*/
            Your("�����͖����ꂽ�D");
        else
/*JP
            pline("Water sprays all over you.");
*/
            pline("�����Ԃ������Ȃ��ɂ��������D");
    }
}

STATIC_PTR void
gush(x, y, poolcnt)
int x, y;
genericptr_t poolcnt;
{
    register struct monst *mtmp;
    register struct trap *ttmp;

    if (((x + y) % 2) || (x == u.ux && y == u.uy)
        || (rn2(1 + distmin(u.ux, u.uy, x, y))) || (levl[x][y].typ != ROOM)
        || (sobj_at(BOULDER, x, y)) || nexttodoor(x, y))
        return;

    if ((ttmp = t_at(x, y)) != 0 && !delfloortrap(ttmp))
        return;

    if (!((*(int *) poolcnt)++))
/*JP
        pline("Water gushes forth from the overflowing fountain!");
*/
        pline("�򂩂琅���ǂǂ��ƈ��o���I");

    /* Put a pool at x, y */
    levl[x][y].typ = POOL;
    /* No kelp! */
    del_engr_at(x, y);
    water_damage_chain(level.objects[x][y], TRUE);

    if ((mtmp = m_at(x, y)) != 0)
        (void) minliquid(mtmp);
    else
        newsym(x, y);
}

/* Find a gem in the sparkling waters. */
STATIC_OVL void
dofindgem()
{
    if (!Blind)
/*JP
        You("spot a gem in the sparkling waters!");
*/
        pline("����߂����̒��ɕ�΂��������I");
    else
/*JP
        You_feel("a gem here!");
*/
        You_feel("��΂�����悤���I");
    (void) mksobj_at(rnd_class(DILITHIUM_CRYSTAL, LUCKSTONE - 1), u.ux, u.uy,
                     FALSE, FALSE);
    SET_FOUNTAIN_LOOTED(u.ux, u.uy);
    newsym(u.ux, u.uy);
    exercise(A_WIS, TRUE); /* a discovery! */
}

void
dryup(x, y, isyou)
xchar x, y;
boolean isyou;
{
    if (IS_FOUNTAIN(levl[x][y].typ)
        && (!rn2(3) || FOUNTAIN_IS_WARNED(x, y))) {
        if (isyou && in_town(x, y) && !FOUNTAIN_IS_WARNED(x, y)) {
            struct monst *mtmp;

            SET_FOUNTAIN_WARNED(x, y);
            /* Warn about future fountain use. */
            for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
                if (DEADMONSTER(mtmp))
                    continue;
                if (is_watch(mtmp->data) && couldsee(mtmp->mx, mtmp->my)
                    && mtmp->mpeaceful) {
                    if (!Deaf) {
/*JP
                    pline("%s yells:", Amonnam(mtmp));
*/
                    pline("%s�͋��񂾁F", Amonnam(mtmp));
/*JP
                    verbalize("Hey, stop using that fountain!");
*/
                    verbalize("�����C��������ȁI");
                    } else {
                        pline("%s earnestly %s %s %s!",
                              Amonnam(mtmp),
                              nolimbs(mtmp->data) ? "shakes" : "waves",
                              mhis(mtmp),
                              nolimbs(mtmp->data)
                                      ? mbodypart(mtmp, HEAD)
                                      : makeplural(mbodypart(mtmp, ARM)));
                    }
                    break;
                }
            }
            /* You can see or hear this effect */
            if (!mtmp)
/*JP
                pline_The("flow reduces to a trickle.");
*/
                pline("����͂���낿���ɂȂ����D");
            return;
        }
        if (isyou && wizard) {
/*JP
            if (yn("Dry up fountain?") == 'n')
*/
            if (yn("������オ�点�܂����H") == 'n')
                return;
        }
        /* replace the fountain with ordinary floor */
        levl[x][y].typ = ROOM;
        levl[x][y].looted = 0;
        levl[x][y].blessedftn = 0;
        if (cansee(x, y))
/*JP
            pline_The("fountain dries up!");
*/
            pline("��͊��オ�����I");
        /* The location is seen if the hero/monster is invisible
           or felt if the hero is blind. */
        newsym(x, y);
        level.flags.nfountains--;
        if (isyou && in_town(x, y))
            (void) angry_guards(FALSE);
    }
}

void
drinkfountain()
{
    /* What happens when you drink from a fountain? */
    register boolean mgkftn = (levl[u.ux][u.uy].blessedftn == 1);
    register int fate = rnd(30);

    if (Levitation) {
/*JP
        floating_above("fountain");
*/
        floating_above("��");
        return;
    }

    if (mgkftn && u.uluck >= 0 && fate >= 10) {
        int i, ii, littleluck = (u.uluck < 4);

/*JP
        pline("Wow!  This makes you feel great!");
*/
        pline("���H�I�ƂĂ��C�����悭�Ȃ����I");
        /* blessed restore ability */
        for (ii = 0; ii < A_MAX; ii++)
            if (ABASE(ii) < AMAX(ii)) {
                ABASE(ii) = AMAX(ii);
                context.botl = 1;
            }
        /* gain ability, blessed if "natural" luck is high */
        i = rn2(A_MAX); /* start at a random attribute */
        for (ii = 0; ii < A_MAX; ii++) {
            if (adjattrib(i, 1, littleluck ? -1 : 0) && littleluck)
                break;
            if (++i >= A_MAX)
                i = 0;
        }
        display_nhwindow(WIN_MESSAGE, FALSE);
/*JP
        pline("A wisp of vapor escapes the fountain...");
*/
        pline("���̂����܂肪�򂩂瓦�����D�D�D");
        exercise(A_WIS, TRUE);
        levl[u.ux][u.uy].blessedftn = 0;
        return;
    }

    if (fate < 10) {
/*JP
        pline_The("cool draught refreshes you.");
*/
        pline("�₽����t�ł����ς肵���D");
        u.uhunger += rnd(10); /* don't choke on water */
        newuhs(FALSE);
        if (mgkftn)
            return;
    } else {
        switch (fate) {
        case 19: /* Self-knowledge */
/*JP
            You_feel("self-knowledgeable...");
*/
            You("�������g������悤�ȋC�������D�D�D");
            display_nhwindow(WIN_MESSAGE, FALSE);
            enlightenment(MAGICENLIGHTENMENT, ENL_GAMEINPROGRESS);
            exercise(A_WIS, TRUE);
/*JP
            pline_The("feeling subsides.");
*/
            pline("���̊����͂Ȃ��Ȃ����D");
            break;
        case 20: /* Foul water */
/*JP
            pline_The("water is foul!  You gag and vomit.");
*/
            pline("���͂Ђǂ��s���Ȗ��������I���Ȃ��͓f���߂����D");
            morehungry(rn1(20, 11));
            vomit();
            break;
        case 21: /* Poisonous */
/*JP
            pline_The("water is contaminated!");
*/
            pline("���͉�������Ă���I");
            if (Poison_resistance) {
/*JP
                pline("Perhaps it is runoff from the nearby %s farm.",
*/
                pline("���Ԃ�C����͋߂���%s�̔_�ꂩ�痬��Ă���D",
                      fruitname(FALSE));
/*JP
                losehp(rnd(4), "unrefrigerated sip of juice", KILLED_BY_AN);
*/
                losehp(rnd(4),"�������ʏ`�̂��������", KILLED_BY_AN);
                break;
            }
            losestr(rn1(4, 3));
/*JP
            losehp(rnd(10), "contaminated water", KILLED_BY);
*/
            losehp(rnd(10),"�������ꂽ����", KILLED_BY);
            exercise(A_CON, FALSE);
            break;
        case 22: /* Fountain of snakes! */
            dowatersnakes();
            break;
        case 23: /* Water demon */
            dowaterdemon();
            break;
        case 24: /* Curse an item */ {
            register struct obj *obj;

/*JP
            pline("This water's no good!");
*/
            pline("���̐��͂ƂĂ��܂����I");
            morehungry(rn1(20, 11));
            exercise(A_CON, FALSE);
            for (obj = invent; obj; obj = obj->nobj)
                if (!rn2(5))
                    curse(obj);
            break;
        }
        case 25: /* See invisible */
            if (Blind) {
                if (Invisible) {
/*JP
                    You("feel transparent.");
*/
                    You("�����ɂȂ����C������D");
                } else {
/*JP
                    You("feel very self-conscious.");
*/
                    You("���ӎ��ߏ�Ɋ������D");
/*JP
                    pline("Then it passes.");
*/
                    pline("���̊����͏������D");
                }
            } else {
/*JP
                You_see("an image of someone stalking you.");
*/
                You("�����������̌�����Ă���f���������D");
/*JP
                pline("But it disappears.");
*/
                pline("�������C����͏����Ă��܂����D");
            }
            HSee_invisible |= FROMOUTSIDE;
            newsym(u.ux, u.uy);
            exercise(A_WIS, TRUE);
            break;
        case 26: /* See Monsters */
            (void) monster_detect((struct obj *) 0, 0);
            exercise(A_WIS, TRUE);
            break;
        case 27: /* Find a gem in the sparkling waters. */
            if (!FOUNTAIN_IS_LOOTED(u.ux, u.uy)) {
                dofindgem();
                break;
            }
            /*FALLTHRU*/
        case 28: /* Water Nymph */
            dowaternymph();
            break;
        case 29: /* Scare */
        {
            register struct monst *mtmp;

#if 0 /*JP:T*/
            pline("This %s gives you bad breath!",
                  hliquid("water"));
#else
            pline("%s�����񂾂瑧���L���Ȃ����I",
                  hliquid("��"));
#endif
            for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
                if (DEADMONSTER(mtmp))
                    continue;
                monflee(mtmp, 0, FALSE, FALSE);
            }
            break;
        }
        case 30: /* Gushing forth in this room */
            dogushforth(TRUE);
            break;
        default:
#if 0 /*JP*/
            pline("This tepid %s is tasteless.",
                  hliquid("water"));
#else
            pline("���̂Ȃ܂ʂ邢%s�͖����Ȃ��D",
                  hliquid("��"));
#endif
            break;
        }
    }
    dryup(u.ux, u.uy, TRUE);
}

void
dipfountain(obj)
register struct obj *obj;
{
    if (Levitation) {
/*JP
        floating_above("fountain");
*/
        floating_above("��");
        return;
    }

    /* Don't grant Excalibur when there's more than one object.  */
    /* (quantity could be > 1 if merged daggers got polymorphed) */
    if (obj->otyp == LONG_SWORD && obj->quan == 1L && u.ulevel >= 5 && !rn2(6)
        && !obj->oartifact
        && !exist_artifact(LONG_SWORD, artiname(ART_EXCALIBUR))) {
        if (u.ualign.type != A_LAWFUL) {
            /* Ha!  Trying to cheat her. */
#if 0 /*JP*/
            pline("A freezing mist rises from the %s and envelopes the sword.",
                  hliquid("water"));
#else
            pline("�₽������%s���痧������C�����񂾁D",
                  hliquid("��"));
#endif
/*JP
            pline_The("fountain disappears!");
*/
            pline("��͏����Ă��܂����I");
            curse(obj);
            if (obj->spe > -6 && !rn2(3))
                obj->spe--;
            obj->oerodeproof = FALSE;
            exercise(A_WIS, FALSE);
        } else {
            /* The lady of the lake acts! - Eric Backus */
            /* Be *REAL* nice */
            pline(
/*JP
              "From the murky depths, a hand reaches up to bless the sword.");
*/
              "�ɂ������[�݂���C�����j������Ǝ肪�L�тĂ����D");
/*JP
            pline("As the hand retreats, the fountain disappears!");
*/
            pline("�肪�ނ��ƁC��͏����Ă��܂����I");
            obj = oname(obj, artiname(ART_EXCALIBUR));
            discover_artifact(ART_EXCALIBUR);
            bless(obj);
            obj->oeroded = obj->oeroded2 = 0;
            obj->oerodeproof = TRUE;
            exercise(A_WIS, TRUE);
        }
        update_inventory();
        levl[u.ux][u.uy].typ = ROOM;
        levl[u.ux][u.uy].looted = 0;
        newsym(u.ux, u.uy);
        level.flags.nfountains--;
        if (in_town(u.ux, u.uy))
            (void) angry_guards(FALSE);
        return;
    } else {
        int er = water_damage(obj, NULL, TRUE);

        if (obj->otyp == POT_ACID
            && er != ER_DESTROYED) { /* Acid and water don't mix */
            useup(obj);
            return;
        } else if (er != ER_NOTHING && !rn2(2)) { /* no further effect */
            return;
        }
    }

    switch (rnd(30)) {
    case 16: /* Curse the item */
        curse(obj);
        break;
    case 17:
    case 18:
    case 19:
    case 20: /* Uncurse the item */
        if (obj->cursed) {
            if (!Blind)
/*JP
                pline_The("%s glows for a moment.", hliquid("water"));
*/
                pline_The("%s�͋P���������D", hliquid("��"));
            uncurse(obj);
        } else {
/*JP
            pline("A feeling of loss comes over you.");
*/
            pline("��ȒE�͊������Ȃ������������D");
        }
        break;
    case 21: /* Water Demon */
        dowaterdemon();
        break;
    case 22: /* Water Nymph */
        dowaternymph();
        break;
    case 23: /* an Endless Stream of Snakes */
        dowatersnakes();
        break;
    case 24: /* Find a gem */
        if (!FOUNTAIN_IS_LOOTED(u.ux, u.uy)) {
            dofindgem();
            break;
        }
        /*FALLTHRU*/
    case 25: /* Water gushes forth */
        dogushforth(FALSE);
        break;
    case 26: /* Strange feeling */
/*JP
        pline("A strange tingling runs up your %s.", body_part(ARM));
*/
        pline("��Ȃ��тꂪ���Ȃ���%s�ɑ������D", body_part(ARM));
        break;
    case 27: /* Strange feeling */
/*JP
        You_feel("a sudden chill.");
*/
        You("�ˑR�������������D");
        break;
    case 28: /* Strange feeling */
/*JP
        pline("An urge to take a bath overwhelms you.");
*/
        pline("���C�ɓ��肽���Ƃ����~�]�ɂ���ꂽ�D");
        {
            long money = money_cnt(invent);
            struct obj *otmp;
            if (money > 10) {
                /* Amount to lose.  Might get rounded up as fountains don't
                 * pay change... */
                money = somegold(money) / 10;
                for (otmp = invent; otmp && money > 0; otmp = otmp->nobj)
                    if (otmp->oclass == COIN_CLASS) {
                        int denomination = objects[otmp->otyp].oc_cost;
                        long coin_loss =
                            (money + denomination - 1) / denomination;
                        coin_loss = min(coin_loss, otmp->quan);
                        otmp->quan -= coin_loss;
                        money -= coin_loss * denomination;
                        if (!otmp->quan)
                            delobj(otmp);
                    }
/*JP
                You("lost some of your money in the fountain!");
*/
                You("���݂𐔖��C��ɗ��Ƃ��Ă��܂����I");
                CLEAR_FOUNTAIN_LOOTED(u.ux, u.uy);
                exercise(A_WIS, FALSE);
            }
        }
        break;
    case 29: /* You see coins */
        /* We make fountains have more coins the closer you are to the
         * surface.  After all, there will have been more people going
         * by.  Just like a shopping mall!  Chris Woodbury  */

        if (FOUNTAIN_IS_LOOTED(u.ux, u.uy))
            break;
        SET_FOUNTAIN_LOOTED(u.ux, u.uy);
        (void) mkgold((long) (rnd((dunlevs_in_dungeon(&u.uz) - dunlev(&u.uz)
                                   + 1) * 2) + 5),
                      u.ux, u.uy);
        if (!Blind)
#if 0 /*JP:T*/
            pline("Far below you, you see coins glistening in the %s.",
                  hliquid("water"));
#else
            pline("�y�����ŁA%s�̒��ŋ��݂��P���Ă���̂��݂����D",
                  hliquid("��"));
#endif
        exercise(A_WIS, TRUE);
        newsym(u.ux, u.uy);
        break;
    }
    update_inventory();
    dryup(u.ux, u.uy, TRUE);
}

void
breaksink(x, y)
int x, y;
{
    if (cansee(x, y) || (x == u.ux && y == u.uy))
/*JP
        pline_The("pipes break!  Water spurts out!");
*/
        pline("�z�ǂ���ꐅ�����o�����I");
    level.flags.nsinks--;
    levl[x][y].doormask = 0;
    levl[x][y].typ = FOUNTAIN;
    level.flags.nfountains++;
    newsym(x, y);
}

void
drinksink()
{
    struct obj *otmp;
    struct monst *mtmp;

    if (Levitation) {
/*JP
        floating_above("sink");
*/
        floating_above("������");
        return;
    }
    switch (rn2(20)) {
    case 0:
/*JP
        You("take a sip of very cold %s.", hliquid("water"));
*/
        You("�ƂĂ��₽��%s��������񂾁D", hliquid("��"));
        break;
    case 1:
/*JP
        You("take a sip of very warm %s.", hliquid("water"));
*/
        You("�ƂĂ�����������%s��������񂾁D", hliquid("��"));
        break;
    case 2:
/*JP
        You("take a sip of scalding hot %s.", hliquid("water"));
*/
        You("�ƂĂ��M��%s��������񂾁D", hliquid("��"));
        if (Fire_resistance)
/*JP
            pline("It seems quite tasty.");
*/
            pline("�ƂĂ��������������D");
        else
/*JP
            losehp(rnd(6), "sipping boiling water", KILLED_BY);
*/
            losehp(rnd(6), "�����������������", KILLED_BY);
        /* boiling water burns considered fire damage */
        break;
    case 3:
        if (mvitals[PM_SEWER_RAT].mvflags & G_GONE)
/*JP
            pline_The("sink seems quite dirty.");
*/
            pline("������͂ƂĂ����Ȃ炵���D");
        else {
            mtmp = makemon(&mons[PM_SEWER_RAT], u.ux, u.uy, NO_MM_FLAGS);
            if (mtmp)
#if 0 /*JP*/
                pline("Eek!  There's %s in the sink!",
                      (Blind || !canspotmon(mtmp)) ? "something squirmy"
                                                   : a_monnam(mtmp));
#else
                pline("���I�������%s������I",
                      (Blind || !canspotmon(mtmp)) ? "�g����������悤�Ȃ���"
                                                   : a_monnam(mtmp));
#endif
        }
        break;
    case 4:
        do {
            otmp = mkobj(POTION_CLASS, FALSE);
            if (otmp->otyp == POT_WATER) {
                obfree(otmp, (struct obj *) 0);
                otmp = (struct obj *) 0;
            }
        } while (!otmp);
        otmp->cursed = otmp->blessed = 0;
#if 0 /*JP*/
        pline("Some %s liquid flows from the faucet.",
              Blind ? "odd" : hcolor(OBJ_DESCR(objects[otmp->otyp])));
#else
        pline("�֌�����%s�t�̂����ꂽ�D",
              Blind ? "���" :
              hcolor(OBJ_DESCR(objects[otmp->otyp])));
#endif
        otmp->dknown = !(Blind || Hallucination);
        otmp->quan++;       /* Avoid panic upon useup() */
        otmp->fromsink = 1; /* kludge for docall() */
        (void) dopotion(otmp);
        obfree(otmp, (struct obj *) 0);
        break;
    case 5:
        if (!(levl[u.ux][u.uy].looted & S_LRING)) {
/*JP
            You("find a ring in the sink!");
*/
            You("������Ɏw�ւ��݂����I");
            (void) mkobj_at(RING_CLASS, u.ux, u.uy, TRUE);
            levl[u.ux][u.uy].looted |= S_LRING;
            exercise(A_WIS, TRUE);
            newsym(u.ux, u.uy);
        } else
/*JP
            pline("Some dirty %s backs up in the drain.", hliquid("water"));
*/
            pline("����%s���r��������t�����Ă����D", hliquid("��"));
        break;
    case 6:
        breaksink(u.ux, u.uy);
        break;
    case 7:
/*JP
        pline_The("%s moves as though of its own will!", hliquid("water"));
*/
        pline_The("%s���ӎv�������Ă��邩�̂悤�ɓ������I", hliquid("��"));
        if ((mvitals[PM_WATER_ELEMENTAL].mvflags & G_GONE)
            || !makemon(&mons[PM_WATER_ELEMENTAL], u.ux, u.uy, NO_MM_FLAGS))
/*JP
            pline("But it quiets down.");
*/
            pline("�������C�Â��ɂȂ����D");
        break;
    case 8:
/*JP
        pline("Yuk, this %s tastes awful.", hliquid("water"));
*/
        pline("�I�F�C����%s�͂ƂĂ��Ђǂ���������D", hliquid("��"));
        more_experienced(1, 0);
        newexplevel();
        break;
    case 9:
/*JP
        pline("Gaggg... this tastes like sewage!  You vomit.");
*/
        pline("�Q�F�[�D�����̂悤�Ȗ�������I���Ȃ��͓f���߂����D");
        morehungry(rn1(30 - ACURR(A_CON), 11));
        vomit();
        break;
    case 10:
/*JP
        pline("This %s contains toxic wastes!", hliquid("water"));
*/
        pline("����%s�͗L�łȔp�������܂�ł���I", hliquid("��"));
        if (!Unchanging) {
/*JP
            You("undergo a freakish metamorphosis!");
*/
            You("��`�ȕω������͂��߂��I");
            polyself(0);
        }
        break;
    /* more odd messages --JJB */
    case 11:
/*JP
        You_hear("clanking from the pipes...");
*/
        You_hear("�z�ǂ̃J�`���Ƃ������𕷂����D�D�D");
        break;
    case 12:
/*JP
        You_hear("snatches of song from among the sewers...");
*/
        You_hear("�����̒�����Ƃ���Ƃ���̉̂𕷂����D�D�D");
        break;
    case 19:
        if (Hallucination) {
/*JP
            pline("From the murky drain, a hand reaches up... --oops--");
*/
            pline("�Â��r��������C�肪�L�тĂ����D�D--������--");
            break;
        }
        /*FALLTHRU*/
    default:
#if 0 /*JP*/
        You("take a sip of %s %s.",
            rn2(3) ? (rn2(2) ? "cold" : "warm") : "hot",
            hliquid("water"));
#else
        You("%s%s��������񂾁D",
            rn2(3) ? (rn2(2) ? "�₽��" : "����������") : "�M��",
            hliquid("��"));
#endif
    }
}

/*fountain.c*/
