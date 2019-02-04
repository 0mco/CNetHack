/* NetHack 3.6	lock.c	$NHDT-Date: 1521499715 2018/03/19 22:48:35 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.80 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2011. */
/* NetHack may be freely redistributed.  See license for details. */

/* JNetHack Copyright */
/* (c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994-2000  */
/* For 3.4-, Copyright (c) SHIRAKATA Kentaro, 2002-2018            */
/* JNetHack may be freely redistributed.  See license for details. */

#include "hack.h"

/* at most one of `door' and `box' should be non-null at any given time */
STATIC_VAR NEARDATA struct xlock_s {
    struct rm *door;
    struct obj *box;
    int picktyp, /* key|pick|card for unlock, sharp vs blunt for #force */
        chance, usedtime;
    boolean magic_key;
} xlock;

/* occupation callbacks */
STATIC_PTR int NDECL(picklock);
STATIC_PTR int NDECL(forcelock);

STATIC_DCL const char *NDECL(lock_action);
STATIC_DCL boolean FDECL(obstructed, (int, int, BOOLEAN_P));
STATIC_DCL void FDECL(chest_shatter_msg, (struct obj *));

boolean
picking_lock(x, y)
int *x, *y;
{
    if (occupation == picklock) {
        *x = u.ux + u.dx;
        *y = u.uy + u.dy;
        return TRUE;
    } else {
        *x = *y = 0;
        return FALSE;
    }
}

boolean
picking_at(x, y)
int x, y;
{
    return (boolean) (occupation == picklock && xlock.door == &levl[x][y]);
}

/* produce an occupation string appropriate for the current activity */
STATIC_OVL const char *
lock_action()
{
    /* "unlocking"+2 == "locking" */
    static const char *actions[] = {
#if 0 /*JP*/
        "unlocking the door",   /* [0] */
        "unlocking the chest",  /* [1] */
        "unlocking the box",    /* [2] */
        "picking the lock"      /* [3] */
#else 
        "���̌����͂���", 
        "�󔠂̌����͂���",
        "���̌����͂���",
        "�����͂���"
#endif
    };

    /* if the target is currently unlocked, we're trying to lock it now */
    if (xlock.door && !(xlock.door->doormask & D_LOCKED))
#if 0 /*JP*/
        return actions[0] + 2; /* "locking the door" */
#else /* �p��� un �����΋t�̈Ӗ��ɂȂ邪�C���{��͂����͂����Ȃ��̂Ń��e���������� */
        return "���Ɍ���������";
#endif
    else if (xlock.box && !xlock.box->olocked)
#if 0 /*JP*/
        return xlock.box->otyp == CHEST ? actions[1] + 2 : actions[2] + 2;
#else
        return xlock.box->otyp == CHEST ? "�󔠂Ɍ���������" : "���Ɍ���������";
#endif
    /* otherwise we're trying to unlock it */
    else if (xlock.picktyp == LOCK_PICK)
        return actions[3]; /* "picking the lock" */
    else if (xlock.picktyp == CREDIT_CARD)
        return actions[3]; /* same as lock_pick */
    else if (xlock.door)
        return actions[0]; /* "unlocking the door" */
    else if (xlock.box)
        return xlock.box->otyp == CHEST ? actions[1] : actions[2];
    else
        return actions[3];
}

/* try to open/close a lock */
STATIC_PTR int
picklock(VOID_ARGS)
{
    if (xlock.box) {
        if (xlock.box->where != OBJ_FLOOR
            || xlock.box->ox != u.ux || xlock.box->oy != u.uy) {
            return ((xlock.usedtime = 0)); /* you or it moved */
        }
    } else { /* door */
        if (xlock.door != &(levl[u.ux + u.dx][u.uy + u.dy])) {
            return ((xlock.usedtime = 0)); /* you moved */
        }
        switch (xlock.door->doormask) {
        case D_NODOOR:
/*JP
            pline("This doorway has no door.");
*/
            pline("�o�����ɂ͔����Ȃ��D");
            return ((xlock.usedtime = 0));
        case D_ISOPEN:
/*JP
            You("cannot lock an open door.");
*/
            pline("�J���Ă���Ɍ����������Ȃ��D");
            return ((xlock.usedtime = 0));
        case D_BROKEN:
/*JP
            pline("This door is broken.");
*/
            pline("���͉��Ă���D");
            return ((xlock.usedtime = 0));
        }
    }

    if (xlock.usedtime++ >= 50 || nohands(youmonst.data)) {
/*JP
        You("give up your attempt at %s.", lock_action());
*/
        pline("%s�̂�������߂��D", lock_action());
        exercise(A_DEX, TRUE); /* even if you don't succeed */
        return ((xlock.usedtime = 0));
    }

    if (rn2(100) >= xlock.chance)
        return 1; /* still busy */

    /* using the Master Key of Thievery finds traps if its bless/curse
       state is adequate (non-cursed for rogues, blessed for others;
       checked when setting up 'xlock') */
    if ((!xlock.door ? (int) xlock.box->otrapped
                     : (xlock.door->doormask & D_TRAPPED) != 0)
        && xlock.magic_key) {
        xlock.chance += 20; /* less effort needed next time */
        /* unfortunately we don't have a 'tknown' flag to record
           "known to be trapped" so declining to disarm and then
           retrying lock manipulation will find it all over again */
/*JP
        if (yn("You find a trap!  Do you want to try to disarm it?") == 'y') {
*/
        if (yn("㩂��������I�O���܂����H") == 'y') {
            const char *what;
            boolean alreadyunlocked;

            /* disarming while using magic key always succeeds */
            if (xlock.door) {
                xlock.door->doormask &= ~D_TRAPPED;
/*JP
                what = "door";
*/
                what = "��";
                alreadyunlocked = !(xlock.door->doormask & D_LOCKED);
            } else {
                xlock.box->otrapped = 0;
/*JP
                what = (xlock.box->otyp == CHEST) ? "chest" : "box";
*/
                what = (xlock.box->otyp == CHEST) ? "��" : "��";
                alreadyunlocked = !xlock.box->olocked;
            }
#if 0 /*JP*/
            You("succeed in disarming the trap.  The %s is still %slocked.",
                what, alreadyunlocked ? "un" : "");
#else
            You("㩂��O�����D%s�͌���%s�܂܂��D",
                what, alreadyunlocked ? "�J����" : "��������");
#endif
            exercise(A_WIS, TRUE);
        } else {
/*JP
            You("stop %s.", lock_action());
*/
            You("%s�̂���߂��D", lock_action());
            exercise(A_WIS, FALSE);
        }
        return ((xlock.usedtime = 0));
    }

/*JP
    You("succeed in %s.", lock_action());
*/
    You("%s�̂ɐ��������D", lock_action());
    if (xlock.door) {
        if (xlock.door->doormask & D_TRAPPED) {
/*JP
            b_trapped("door", FINGER);
*/
            b_trapped("��", FINGER);
            xlock.door->doormask = D_NODOOR;
            unblock_point(u.ux + u.dx, u.uy + u.dy);
            if (*in_rooms(u.ux + u.dx, u.uy + u.dy, SHOPBASE))
                add_damage(u.ux + u.dx, u.uy + u.dy, SHOP_DOOR_COST);
            newsym(u.ux + u.dx, u.uy + u.dy);
        } else if (xlock.door->doormask & D_LOCKED)
            xlock.door->doormask = D_CLOSED;
        else
            xlock.door->doormask = D_LOCKED;
    } else {
        xlock.box->olocked = !xlock.box->olocked;
        xlock.box->lknown = 1;
        if (xlock.box->otrapped)
            (void) chest_trap(xlock.box, FINGER, FALSE);
    }
    exercise(A_DEX, TRUE);
    return ((xlock.usedtime = 0));
}

void
breakchestlock(box, destroyit)
struct obj *box;
boolean destroyit;
{
    if (!destroyit) { /* bill for the box but not for its contents */
        struct obj *hide_contents = box->cobj;

        box->cobj = 0;
        costly_alteration(box, COST_BRKLCK);
        box->cobj = hide_contents;
        box->olocked = 0;
        box->obroken = 1;
        box->lknown = 1;
    } else { /* #force has destroyed this box (at <u.ux,u.uy>) */
        struct obj *otmp;
        struct monst *shkp = (*u.ushops && costly_spot(u.ux, u.uy))
                                 ? shop_keeper(*u.ushops)
                                 : 0;
        boolean costly = (boolean) (shkp != 0),
                peaceful_shk = costly && (boolean) shkp->mpeaceful;
        long loss = 0L;

/*JP
        pline("In fact, you've totally destroyed %s.", the(xname(box)));
*/
        pline("���ۂ̂Ƃ���C%s�����S�ɉ󂵂Ă��܂����D", xname(xlock.box));
        /* Put the contents on ground at the hero's feet. */
        while ((otmp = box->cobj) != 0) {
            obj_extract_self(otmp);
            if (!rn2(3) || otmp->oclass == POTION_CLASS) {
                chest_shatter_msg(otmp);
                if (costly)
                    loss +=
                        stolen_value(otmp, u.ux, u.uy, peaceful_shk, TRUE);
                if (otmp->quan == 1L) {
                    obfree(otmp, (struct obj *) 0);
                    continue;
                }
                useup(otmp);
            }
            if (box->otyp == ICE_BOX && otmp->otyp == CORPSE) {
                otmp->age = monstermoves - otmp->age; /* actual age */
                start_corpse_timeout(otmp);
            }
            place_object(otmp, u.ux, u.uy);
            stackobj(otmp);
        }
        if (costly)
            loss += stolen_value(box, u.ux, u.uy, peaceful_shk, TRUE);
        if (loss)
/*JP
            You("owe %ld %s for objects destroyed.", loss, currency(loss));
*/
            You("�함�j����%ld%s�̎؂���������D", loss, currency(loss));
        delobj(box);
    }
}

/* try to force a locked chest */
STATIC_PTR int
forcelock(VOID_ARGS)
{
    if ((xlock.box->ox != u.ux) || (xlock.box->oy != u.uy))
        return ((xlock.usedtime = 0)); /* you or it moved */

    if (xlock.usedtime++ >= 50 || !uwep || nohands(youmonst.data)) {
/*JP
        You("give up your attempt to force the lock.");
*/
        pline("���������J����̂�������߂��D");
        if (xlock.usedtime >= 50) /* you made the effort */
            exercise((xlock.picktyp) ? A_DEX : A_STR, TRUE);
        return ((xlock.usedtime = 0));
    }

    if (xlock.picktyp) { /* blade */
        if (rn2(1000 - (int) uwep->spe) > (992 - greatest_erosion(uwep) * 10)
            && !uwep->cursed && !obj_resists(uwep, 0, 99)) {
            /* for a +0 weapon, probability that it survives an unsuccessful
             * attempt to force the lock is (.992)^50 = .67
             */
#if 0 /*JP*/
            pline("%sour %s broke!", (uwep->quan > 1L) ? "One of y" : "Y",
                  xname(uwep));
#else
            pline("%s�͉��Ă��܂����I",xname(uwep));
#endif
            useup(uwep);
/*JP
            You("give up your attempt to force the lock.");
*/
            pline("���������J����̂�������߂��D");
            exercise(A_DEX, TRUE);
            return ((xlock.usedtime = 0));
        }
    } else             /* blunt */
        wake_nearby(); /* due to hammering on the container */

    if (rn2(100) >= xlock.chance)
        return 1; /* still busy */

/*JP
    You("succeed in forcing the lock.");
*/
    pline("���������J�����D");
    breakchestlock(xlock.box, (boolean) (!xlock.picktyp && !rn2(3)));

    exercise((xlock.picktyp) ? A_DEX : A_STR, TRUE);
    return ((xlock.usedtime = 0));
}

void
reset_pick()
{
    xlock.usedtime = xlock.chance = xlock.picktyp = 0;
    xlock.magic_key = FALSE;
    xlock.door = 0;
    xlock.box = 0;
}

/* level change; don't reset if hero is carrying xlock.box with him/her */
void
maybe_reset_pick()
{
    if (!xlock.box || !carried(xlock.box))
        reset_pick();
}

/* for doapply(); if player gives a direction or resumes an interrupted
   previous attempt then it costs hero a move even if nothing ultimately
   happens; when told "can't do that" before being asked for direction
   or player cancels with ESC while giving direction, it doesn't */
#define PICKLOCK_LEARNED_SOMETHING (-1) /* time passes */
#define PICKLOCK_DID_NOTHING 0          /* no time passes */
#define PICKLOCK_DID_SOMETHING 1

/* player is applying a key, lock pick, or credit card */
int
pick_lock(pick)
struct obj *pick;
{
    int picktyp, c, ch;
    coord cc;
    struct rm *door;
    struct obj *otmp;
    char qbuf[QBUFSZ];

    picktyp = pick->otyp;

    /* check whether we're resuming an interrupted previous attempt */
    if (xlock.usedtime && picktyp == xlock.picktyp) {
/*JP
        static char no_longer[] = "Unfortunately, you can no longer %s %s.";
*/
        static char no_longer[] = "����˂�Ȃ���C���Ȃ���%s%s";

        if (nohands(youmonst.data)) {
/*JP
            const char *what = (picktyp == LOCK_PICK) ? "pick" : "key";
*/
            const char *what = (picktyp == LOCK_PICK) ? "���J�����" : "��";

            if (picktyp == CREDIT_CARD)
/*JP
                what = "card";
*/
                what = "�J�[�h";
/*JP
            pline(no_longer, "hold the", what);
*/
            pline(no_longer, what, "�����߂Ȃ�");
            reset_pick();
            return PICKLOCK_LEARNED_SOMETHING;
        } else if (u.uswallow || (xlock.box && !can_reach_floor(TRUE))) {
/*JP
            pline(no_longer, "reach the", "lock");
*/
            pline(no_longer, "����", "�͂��Ȃ�");
            reset_pick();
            return PICKLOCK_LEARNED_SOMETHING;
        } else {
            const char *action = lock_action();

/*JP
            You("resume your attempt at %s.", action);
*/
            pline("%s�̂��ĊJ�����D", action);
            xlock.magic_key = is_magic_key(&youmonst, pick);
            set_occupation(picklock, action, 0);
            return PICKLOCK_DID_SOMETHING;
        }
    }

    if (nohands(youmonst.data)) {
/*JP
        You_cant("hold %s -- you have no hands!", doname(pick));
*/
        You("%s�����ނ��Ƃ��ł��Ȃ��I�肪�Ȃ��񂾂��́I", xname(pick));
        return PICKLOCK_DID_NOTHING;
    } else if (u.uswallow) {
#if 0 /*JP*/
        You_cant("%sunlock %s.", (picktyp == CREDIT_CARD) ? "" : "lock or ",
                 mon_nam(u.ustuck));
#else
        You_cant("%s��%s�Ȃ��D", mon_nam(u.ustuck),
                 (picktyp == CREDIT_CARD) ? "�J�����" : "�J���߂ł�");
#endif
        return PICKLOCK_DID_NOTHING;
    }

    if (picktyp != LOCK_PICK
        && picktyp != CREDIT_CARD
        && picktyp != SKELETON_KEY) {
        impossible("picking lock with object %d?", picktyp);
        return PICKLOCK_DID_NOTHING;
    }
    ch = 0; /* lint suppression */

/*JP
    if (!get_adjacent_loc((char *) 0, "Invalid location!", u.ux, u.uy, &cc))
*/
    if (!get_adjacent_loc((char *) 0, "�ʒu�����������I", u.ux, u.uy, &cc))
        return PICKLOCK_DID_NOTHING;

    if (cc.x == u.ux && cc.y == u.uy) { /* pick lock on a container */
        const char *verb;
        char qsfx[QBUFSZ];
#if 0 /*JP*/
        boolean it;
#endif
        int count;

        if (u.dz < 0) {
#if 0 /*JP*/
            There("isn't any sort of lock up %s.",
                  Levitation ? "here" : "there");
#else
            pline("%s�ɂ͌���������悤�ȕ��͂Ȃ��D",
                  Levitation ? "����" : "����");
#endif
            return PICKLOCK_LEARNED_SOMETHING;
        } else if (is_lava(u.ux, u.uy)) {
/*JP
            pline("Doing that would probably melt %s.", yname(pick));
*/
            pline("����Ȃ��Ƃ�������%s���n���Ă��܂��D", yname(pick));
            return PICKLOCK_LEARNED_SOMETHING;
        } else if (is_pool(u.ux, u.uy) && !Underwater) {
/*JP
            pline_The("%s has no lock.", hliquid("water"));
*/
            pline_The("%s�ɏ��O�͂Ȃ��D", hliquid("��"));
            return PICKLOCK_LEARNED_SOMETHING;
        }

        count = 0;
        c = 'n'; /* in case there are no boxes here */
        for (otmp = level.objects[cc.x][cc.y]; otmp; otmp = otmp->nexthere)
            if (Is_box(otmp)) {
                ++count;
                if (!can_reach_floor(TRUE)) {
/*JP
                    You_cant("reach %s from up here.", the(xname(otmp)));
*/
                    You("��������%s�ɓ͂��Ȃ��D", the(xname(otmp)));
                    return PICKLOCK_LEARNED_SOMETHING;
                }
#if 0 /*JP*/
                it = 0;
#endif
                if (otmp->obroken)
/*JP
                    verb = "fix";
*/
                    verb = "�C������";
                else if (!otmp->olocked)
/*JP
                    verb = "lock", it = 1;
*/
                    verb = "����������";
                else if (picktyp != LOCK_PICK)
/*JP
                    verb = "unlock", it = 1;
*/
                    verb = "�����͂���";
                else
/*JP
                    verb = "pick";
*/
                    verb = "����������";

                /* "There is <a box> here; <verb> <it|its lock>?" */
/*JP
                Sprintf(qsfx, " here; %s %s?", verb, it ? "it" : "its lock");
*/
                Sprintf(qsfx, "������D%s�H", verb);
#if 0 /*JP*/
                (void) safe_qbuf(qbuf, "There is ", qsfx, otmp, doname,
                                 ansimpleoname, "a box");
#else
                (void) safe_qbuf(qbuf, "�����ɂ�", qsfx, otmp, doname,
                                 ansimpleoname, "��");
#endif
                otmp->lknown = 1;

                c = ynq(qbuf);
                if (c == 'q')
                    return 0;
                if (c == 'n')
                    continue;

                if (otmp->obroken) {
/*JP
                    You_cant("fix its broken lock with %s.", doname(pick));
*/
                    You("��ꂽ����%s�ŏC���ł��Ȃ��D", doname(pick));
                    return PICKLOCK_LEARNED_SOMETHING;
                } else if (picktyp == CREDIT_CARD && !otmp->olocked) {
                    /* credit cards are only good for unlocking */
#if 0 /*JP*/
                    You_cant("do that with %s.",
                             an(simple_typename(picktyp)));
#else
                    pline("%s���Ⴛ��Ȃ��Ƃ͂ł��Ȃ��D",
                          simple_typename(picktyp));
#endif
                    return PICKLOCK_LEARNED_SOMETHING;
                }
                switch (picktyp) {
                case CREDIT_CARD:
                    ch = ACURR(A_DEX) + 20 * Role_if(PM_ROGUE);
                    break;
                case LOCK_PICK:
                    ch = 4 * ACURR(A_DEX) + 25 * Role_if(PM_ROGUE);
                    break;
                case SKELETON_KEY:
                    ch = 75 + ACURR(A_DEX);
                    break;
                default:
                    ch = 0;
                }
                if (otmp->cursed)
                    ch /= 2;

                xlock.box = otmp;
                xlock.door = 0;
                break;
            }
        if (c != 'y') {
            if (!count)
/*JP
                There("doesn't seem to be any sort of lock here.");
*/
                pline("�����ɂ͌���������悤�ȕ��͂Ȃ��悤���D");
            return PICKLOCK_LEARNED_SOMETHING; /* decided against all boxes */
        }
    } else { /* pick the lock in a door */
        struct monst *mtmp;

        if (u.utrap && u.utraptype == TT_PIT) {
/*JP
            You_cant("reach over the edge of the pit.");
*/
            pline("�������̒�����ł͓͂��Ȃ��D");
            return PICKLOCK_LEARNED_SOMETHING;
        }

        door = &levl[cc.x][cc.y];
        mtmp = m_at(cc.x, cc.y);
        if (mtmp && canseemon(mtmp) && mtmp->m_ap_type != M_AP_FURNITURE
            && mtmp->m_ap_type != M_AP_OBJECT) {
            if (picktyp == CREDIT_CARD
                && (mtmp->isshk || mtmp->data == &mons[PM_ORACLE]))
/*JP
                verbalize("No checks, no credit, no problem.");
*/
                verbalize("�����j�R�j�R���������D");
            else
#if 0 /*JP*/
                pline("I don't think %s would appreciate that.",
                      mon_nam(mtmp));
#else
                pline("%s�����̉��l��F�߂�Ƃ͎v���Ȃ��D", mon_nam(mtmp));
#endif
            return PICKLOCK_LEARNED_SOMETHING;
        } else if (mtmp && is_door_mappear(mtmp)) {
            /* "The door actually was a <mimic>!" */
            stumble_onto_mimic(mtmp);
            /* mimic might keep the key (50% chance, 10% for PYEC or MKoT) */
            maybe_absorb_item(mtmp, pick, 50, 10);
            return PICKLOCK_LEARNED_SOMETHING;
        }
        if (!IS_DOOR(door->typ)) {
            if (is_drawbridge_wall(cc.x, cc.y) >= 0)
/*JP
                You("%s no lock on the drawbridge.", Blind ? "feel" : "see");
*/
                pline("���ˋ��ɂ͌����Ȃ�%s�D", Blind ? "�悤��" : "�悤�Ɍ�����");
            else
/*JP
                You("%s no door there.", Blind ? "feel" : "see");
*/
                pline("�����ɂ͔����Ȃ�%s�D", Blind ? "�悤��" : "�悤�Ɍ�����");
            return PICKLOCK_LEARNED_SOMETHING;
        }
        switch (door->doormask) {
        case D_NODOOR:
/*JP
            pline("This doorway has no door.");
*/
            pline("���̏o�����ɂ͔����Ȃ��D");
            return PICKLOCK_LEARNED_SOMETHING;
        case D_ISOPEN:
/*JP
            You("cannot lock an open door.");
*/
            pline("�J���Ă���ɂ͌����������Ȃ��D");
            return PICKLOCK_LEARNED_SOMETHING;
        case D_BROKEN:
/*JP
            pline("This door is broken.");
*/
            pline("���̔��͉��Ă���D");
            return PICKLOCK_LEARNED_SOMETHING;
        default:
            /* credit cards are only good for unlocking */
            if (picktyp == CREDIT_CARD && !(door->doormask & D_LOCKED)) {
/*JP
                You_cant("lock a door with a credit card.");
*/
                You("�N���W�b�g�J�[�h�Ŕ��Ɍ��������邱�Ƃ͂ł��Ȃ��D");
                return PICKLOCK_LEARNED_SOMETHING;
            }

#if 0 /*JP*/
            Sprintf(qbuf, "%s it?",
                    (door->doormask & D_LOCKED) ? "Unlock" : "Lock");
#else
            Sprintf(qbuf, "%s�܂����H",
                    (door->doormask & D_LOCKED) ? "�͂���" : "����" );
#endif

            c = yn(qbuf);
            if (c == 'n')
                return 0;

            switch (picktyp) {
            case CREDIT_CARD:
                ch = 2 * ACURR(A_DEX) + 20 * Role_if(PM_ROGUE);
                break;
            case LOCK_PICK:
                ch = 3 * ACURR(A_DEX) + 30 * Role_if(PM_ROGUE);
                break;
            case SKELETON_KEY:
                ch = 70 + ACURR(A_DEX);
                break;
            default:
                ch = 0;
            }
            xlock.door = door;
            xlock.box = 0;
        }
    }
    context.move = 0;
    xlock.chance = ch;
    xlock.picktyp = picktyp;
    xlock.magic_key = is_magic_key(&youmonst, pick);
    xlock.usedtime = 0;
    set_occupation(picklock, lock_action(), 0);
    return PICKLOCK_DID_SOMETHING;
}

/* try to force a chest with your weapon */
int
doforce()
{
    register struct obj *otmp;
    register int c, picktyp;
    char qbuf[QBUFSZ];

    if (u.uswallow) {
/*JP
        You_cant("force anything from inside here.");
*/
        You_cant("�������炱���J���邱�Ƃ͂ł��Ȃ��D");
        return 0;
    }
    if (!uwep /* proper type test */
        || ((uwep->oclass == WEAPON_CLASS || is_weptool(uwep))
               ? (objects[uwep->otyp].oc_skill < P_DAGGER
                  || objects[uwep->otyp].oc_skill == P_FLAIL
                  || objects[uwep->otyp].oc_skill > P_LANCE)
               : uwep->oclass != ROCK_CLASS)) {
#if 0 /*JP*/
        You_cant("force anything %s weapon.",
                 !uwep ? "when not wielding a"
                       : (uwep->oclass != WEAPON_CLASS && !is_weptool(uwep))
                             ? "without a proper"
                             : "with that");
#else
        You_cant("%s����Ȃ��Ō��������J���邱�Ƃ͂ł��Ȃ��D",
                 !uwep ? "�������Ă���"
                       : (uwep->oclass != WEAPON_CLASS && !is_weptool(uwep))
                             ? "�K�؂�"
                             : "");
#endif
        return 0;
    }
    if (!can_reach_floor(TRUE)) {
        cant_reach_floor(u.ux, u.uy, FALSE, TRUE);
        return 0;
    }

    picktyp = is_blade(uwep) && !is_pick(uwep);
    if (xlock.usedtime && xlock.box && picktyp == xlock.picktyp) {
/*JP
        You("resume your attempt to force the lock.");
*/
        pline("��������������̂��ĊJ�����D");
/*JP
        set_occupation(forcelock, "forcing the lock", 0);
*/
        set_occupation(forcelock, "��������������", 0);
        return 1;
    }

    /* A lock is made only for the honest man, the thief will break it. */
    xlock.box = (struct obj *) 0;
    for (otmp = level.objects[u.ux][u.uy]; otmp; otmp = otmp->nexthere)
        if (Is_box(otmp)) {
            if (otmp->obroken || !otmp->olocked) {
                /* force doname() to omit known "broken" or "unlocked"
                   prefix so that the message isn't worded redundantly;
                   since we're about to set lknown, there's no need to
                   remember and then reset its current value */
                otmp->lknown = 0;
#if 0 /*JP*/
                There("is %s here, but its lock is already %s.",
                      doname(otmp), otmp->obroken ? "broken" : "unlocked");
#else
                pline("�����ɂ�%s������D���������̌��͂���%s�D",
                      doname(otmp),
                      otmp->obroken ? "���Ă���" : "�͂�����Ă���");
#endif
                otmp->lknown = 1;
                continue;
            }
#if 0 /*JP*/
            (void) safe_qbuf(qbuf, "There is ", " here; force its lock?",
                             otmp, doname, ansimpleoname, "a box");
#else
            (void) safe_qbuf(qbuf, "�����ɂ�", "������D���������J���܂����H",
                             otmp, doname, ansimpleoname, "��");
#endif
            otmp->lknown = 1;

            c = ynq(qbuf);
            if (c == 'q')
                return 0;
            if (c == 'n')
                continue;

            if (picktyp)
/*JP
                You("force %s into a crack and pry.", yname(uwep));
*/
                You("%s�������Ԃɍ�������ł��������悤�Ƃ����D",xname(uwep));
            else
/*JP
                You("start bashing it with %s.", yname(uwep));
*/
                pline("%s�ŉ�������D", xname(uwep));
            xlock.box = otmp;
            xlock.chance = objects[uwep->otyp].oc_wldam * 2;
            xlock.picktyp = picktyp;
            xlock.magic_key = FALSE;
            xlock.usedtime = 0;
            break;
        }

    if (xlock.box)
/*JP
        set_occupation(forcelock, "forcing the lock", 0);
*/
        set_occupation(forcelock, "��������������", 0);
    else
/*JP
        You("decide not to force the issue.");
*/
        pline("����͖��Ӗ��ȍs�ׂ��D");
    return 1;
}

boolean
stumble_on_door_mimic(x, y)
int x, y;
{
    struct monst *mtmp;

    if ((mtmp = m_at(x, y)) && is_door_mappear(mtmp)
        && !Protection_from_shape_changers) {
        stumble_onto_mimic(mtmp);
        return TRUE;
    }
    return FALSE;
}

/* the 'O' command - try to open a door */
int
doopen()
{
    return doopen_indir(0, 0);
}

/* try to open a door in direction u.dx/u.dy */
int
doopen_indir(x, y)
int x, y;
{
    coord cc;
    register struct rm *door;
    boolean portcullis;
    int res = 0;

    if (nohands(youmonst.data)) {
/*JP
        You_cant("open anything -- you have no hands!");
*/
        You("�����J���邱�Ƃ��ł��Ȃ��I�肪�Ȃ��񂾂��́I");
        return 0;
    }

    if (u.utrap && u.utraptype == TT_PIT) {
/*JP
        You_cant("reach over the edge of the pit.");
*/
        pline("�������̒�����͂��Ȃ��D");
        return 0;
    }

    if (x > 0 && y > 0) {
        cc.x = x;
        cc.y = y;
    } else if (!get_adjacent_loc((char *) 0, (char *) 0, u.ux, u.uy, &cc))
        return 0;

    /* open at yourself/up/down */
    if ((cc.x == u.ux) && (cc.y == u.uy))
        return doloot();

    if (stumble_on_door_mimic(cc.x, cc.y))
        return 1;

    /* when choosing a direction is impaired, use a turn
       regardless of whether a door is successfully targetted */
    if (Confusion || Stunned)
        res = 1;

    door = &levl[cc.x][cc.y];
    portcullis = (is_drawbridge_wall(cc.x, cc.y) >= 0);
    if (Blind) {
        int oldglyph = door->glyph;
        schar oldlastseentyp = lastseentyp[cc.x][cc.y];

        feel_location(cc.x, cc.y);
        if (door->glyph != oldglyph
            || lastseentyp[cc.x][cc.y] != oldlastseentyp)
            res = 1; /* learned something */
    }

    if (portcullis || !IS_DOOR(door->typ)) {
        /* closed portcullis or spot that opened bridge would span */
        if (is_db_wall(cc.x, cc.y) || door->typ == DRAWBRIDGE_UP)
/*JP
            There("is no obvious way to open the drawbridge.");
*/
            pline("���ˋ����~�낷�����ȕ��@�͂Ȃ��D");
        else if (portcullis || door->typ == DRAWBRIDGE_DOWN)
/*JP
            pline_The("drawbridge is already open.");
*/
            pline_The("���ˋ��͂����J���Ă���D");
        else if (container_at(cc.x, cc.y, TRUE))
#if 0 /*JP*/
            pline("%s like something lootable over there.",
                  Blind ? "Feels" : "Seems");
#else
            pline("�����ɂ͉������ꕨ������悤���D");
#endif
        else
/*JP
            You("%s no door there.", Blind ? "feel" : "see");
*/
            pline("�����ɂ͔��͂Ȃ��悤%s�D", Blind ? "��" : "�Ɍ�����");
        return res;
    }

    if (!(door->doormask & D_CLOSED)) {
        const char *mesg;

        switch (door->doormask) {
        case D_BROKEN:
/*JP
            mesg = " is broken";
*/
            mesg = "���͉��Ă���";
            break;
        case D_NODOOR:
/*JP
            mesg = "way has no door";
*/
            mesg = "�o�����ɂ͔����Ȃ�";
            break;
        case D_ISOPEN:
/*JP
            mesg = " is already open";
*/
            mesg = "���͂����J���Ă���";
            break;
        default:
/*JP
            mesg = " is locked";
*/
            mesg = "���ɂ͌����|�����Ă���";
            break;
        }
/*JP
        pline("This door%s.", mesg);
*/
        pline("%s�D", mesg);
        return res;
    }

    if (verysmall(youmonst.data)) {
/*JP
        pline("You're too small to pull the door open.");
*/
        You("���������ɂ͏���������D");
        return res;
    }

    /* door is known to be CLOSED */
    if (rnl(20) < (ACURRSTR + ACURR(A_DEX) + ACURR(A_CON)) / 3) {
/*JP
        pline_The("door opens.");
*/
        pline("���͊J�����D");
        if (door->doormask & D_TRAPPED) {
/*JP
            b_trapped("door", FINGER);
*/
            b_trapped("��", FINGER);
            door->doormask = D_NODOOR;
            if (*in_rooms(cc.x, cc.y, SHOPBASE))
                add_damage(cc.x, cc.y, SHOP_DOOR_COST);
        } else
            door->doormask = D_ISOPEN;
        feel_newsym(cc.x, cc.y); /* the hero knows she opened it */
        unblock_point(cc.x, cc.y); /* vision: new see through there */
    } else {
        exercise(A_STR, TRUE);
/*JP
        pline_The("door resists!");
*/
        pline("�Ȃ��Ȃ��J���Ȃ��I");
    }

    return 1;
}

STATIC_OVL boolean
obstructed(x, y, quietly)
register int x, y;
boolean quietly;
{
    register struct monst *mtmp = m_at(x, y);

    if (mtmp && mtmp->m_ap_type != M_AP_FURNITURE) {
        if (mtmp->m_ap_type == M_AP_OBJECT)
            goto objhere;
        if (!quietly) {
            if ((mtmp->mx != x) || (mtmp->my != y)) {
                /* worm tail */
#if 0 /*JP*/
                pline("%s%s blocks the way!",
                      !canspotmon(mtmp) ? Something : s_suffix(Monnam(mtmp)),
                      !canspotmon(mtmp) ? "" : " tail");
#else
                pline("%s%s�������ӂ������Ă���I",
                      !canspotmon(mtmp) ? "����" : Monnam(mtmp),
                      !canspotmon(mtmp) ? "" : "�̐K��");
#endif
            } else {
#if 0 /*JP*/
                pline("%s blocks the way!",
                      !canspotmon(mtmp) ? "Some creature" : Monnam(mtmp));
#else
                pline("%s�������ӂ������Ă���I",
                      !canspotmon(mtmp) ? "���҂�" : Monnam(mtmp));
#endif
            }
        }
        if (!canspotmon(mtmp))
            map_invisible(x, y);
        return TRUE;
    }
    if (OBJ_AT(x, y)) {
    objhere:
        if (!quietly)
/*JP
            pline("%s's in the way.", Something);
*/
            pline("�������o�����ɂ���D");
        return TRUE;
    }
    return FALSE;
}

/* the 'C' command - try to close a door */
int
doclose()
{
    register int x, y;
    register struct rm *door;
    boolean portcullis;
    int res = 0;

    if (nohands(youmonst.data)) {
/*JP
        You_cant("close anything -- you have no hands!");
*/
        You("�߂邱�Ƃ��ł��Ȃ��I�肪�Ȃ��񂾂��́I");
        return 0;
    }

    if (u.utrap && u.utraptype == TT_PIT) {
/*JP
        You_cant("reach over the edge of the pit.");
*/
        pline("�������̒�����͂��Ȃ��D");
        return 0;
    }

    if (!getdir((char *) 0))
        return 0;

    x = u.ux + u.dx;
    y = u.uy + u.dy;
    if ((x == u.ux) && (y == u.uy)) {
/*JP
        You("are in the way!");
*/
        pline("���Ȃ����o�����ɂ���̂ŕ܂�Ȃ��I");
        return 1;
    }

    if (!isok(x, y))
        goto nodoor;

    if (stumble_on_door_mimic(x, y))
        return 1;

    /* when choosing a direction is impaired, use a turn
       regardless of whether a door is successfully targetted */
    if (Confusion || Stunned)
        res = 1;

    door = &levl[x][y];
    portcullis = (is_drawbridge_wall(x, y) >= 0);
    if (Blind) {
        int oldglyph = door->glyph;
        schar oldlastseentyp = lastseentyp[x][y];

        feel_location(x, y);
        if (door->glyph != oldglyph || lastseentyp[x][y] != oldlastseentyp)
            res = 1; /* learned something */
    }

    if (portcullis || !IS_DOOR(door->typ)) {
        /* is_db_wall: closed portcullis */
        if (is_db_wall(x, y) || door->typ == DRAWBRIDGE_UP)
/*JP
            pline_The("drawbridge is already closed.");
*/
            pline_The("���ˋ��͂������Ă���D");
        else if (portcullis || door->typ == DRAWBRIDGE_DOWN)
/*JP
            There("is no obvious way to close the drawbridge.");
*/
            pline("���ˋ���߂閾���ȕ��@�͂Ȃ��D");
        else {
        nodoor:
/*JP
            You("%s no door there.", Blind ? "feel" : "see");
*/
            pline("�����ɔ��͂Ȃ��悤%s�D", Blind ? "��" : "�Ɍ�����");
        }
        return res;
    }

    if (door->doormask == D_NODOOR) {
/*JP
        pline("This doorway has no door.");
*/
        pline("�o�����ɂ͔����Ȃ��D");
        return res;
    } else if (obstructed(x, y, FALSE)) {
        return res;
    } else if (door->doormask == D_BROKEN) {
/*JP
        pline("This door is broken.");
*/
        pline("���͉��Ă���D");
        return res;
    } else if (door->doormask & (D_CLOSED | D_LOCKED)) {
/*JP
        pline("This door is already closed.");
*/
        pline("���͂������Ă���D");
        return res;
    }

    if (door->doormask == D_ISOPEN) {
        if (verysmall(youmonst.data) && !u.usteed) {
/*JP
            pline("You're too small to push the door closed.");
*/
            You("���������Ĕ���߂��Ȃ��D");
            return res;
        }
        if (u.usteed
            || rn2(25) < (ACURRSTR + ACURR(A_DEX) + ACURR(A_CON)) / 3) {
/*JP
            pline_The("door closes.");
*/
            pline("���͕����D");
            door->doormask = D_CLOSED;
            feel_newsym(x, y); /* the hero knows she closed it */
            block_point(x, y); /* vision:  no longer see there */
        } else {
            exercise(A_STR, TRUE);
/*JP
            pline_The("door resists!");
*/
            pline("�Ȃ��Ȃ��܂�Ȃ��I");
        }
    }

    return 1;
}

/* box obj was hit with spell or wand effect otmp;
   returns true if something happened */
boolean
boxlock(obj, otmp)
struct obj *obj, *otmp; /* obj *is* a box */
{
    boolean res = 0;

    switch (otmp->otyp) {
    case WAN_LOCKING:
    case SPE_WIZARD_LOCK:
        if (!obj->olocked) { /* lock it; fix if broken */
/*JP
            pline("Klunk!");
*/
            pline("�J�`�I");
            obj->olocked = 1;
            obj->obroken = 0;
            if (Role_if(PM_WIZARD))
                obj->lknown = 1;
            else
                obj->lknown = 0;
            res = 1;
        } /* else already closed and locked */
        break;
    case WAN_OPENING:
    case SPE_KNOCK:
        if (obj->olocked) { /* unlock; couldn't be broken */
/*JP
            pline("Klick!");
*/
            pline("�R���R���I");
            obj->olocked = 0;
            res = 1;
            if (Role_if(PM_WIZARD))
                obj->lknown = 1;
            else
                obj->lknown = 0;
        } else /* silently fix if broken */
            obj->obroken = 0;
        break;
    case WAN_POLYMORPH:
    case SPE_POLYMORPH:
        /* maybe start unlocking chest, get interrupted, then zap it;
           we must avoid any attempt to resume unlocking it */
        if (xlock.box == obj)
            reset_pick();
        break;
    }
    return res;
}

/* Door/secret door was hit with spell or wand effect otmp;
   returns true if something happened */
boolean
doorlock(otmp, x, y)
struct obj *otmp;
int x, y;
{
    register struct rm *door = &levl[x][y];
    boolean res = TRUE;
    int loudness = 0;
    const char *msg = (const char *) 0;
/*JP
    const char *dustcloud = "A cloud of dust";
*/
    const char *dustcloud = "�ق���";
/*JP
    const char *quickly_dissipates = "quickly dissipates";
*/
    const char *quickly_dissipates = "�����ƌ����܂ɔ�юU����";
    boolean mysterywand = (otmp->oclass == WAND_CLASS && !otmp->dknown);

    if (door->typ == SDOOR) {
        switch (otmp->otyp) {
        case WAN_OPENING:
        case SPE_KNOCK:
        case WAN_STRIKING:
        case SPE_FORCE_BOLT:
            door->typ = DOOR;
            door->doormask = D_CLOSED | (door->doormask & D_TRAPPED);
            newsym(x, y);
            if (cansee(x, y))
/*JP
                pline("A door appears in the wall!");
*/
                pline("�ǂ���������ꂽ�I");
            if (otmp->otyp == WAN_OPENING || otmp->otyp == SPE_KNOCK)
                return TRUE;
            break; /* striking: continue door handling below */
        case WAN_LOCKING:
        case SPE_WIZARD_LOCK:
        default:
            return FALSE;
        }
    }

    switch (otmp->otyp) {
    case WAN_LOCKING:
    case SPE_WIZARD_LOCK:
        if (Is_rogue_level(&u.uz)) {
            boolean vis = cansee(x, y);
            /* Can't have real locking in Rogue, so just hide doorway */
            if (vis)
/*JP
                pline("%s springs up in the older, more primitive doorway.",
*/
                pline("�Â������C���n�I�ȏo������%s���������߂��D",
                      dustcloud);
            else
/*JP
                You_hear("a swoosh.");
*/
                You_hear("�V���[�b�Ƃ������𕷂����D");
            if (obstructed(x, y, mysterywand)) {
                if (vis)
/*JP
                    pline_The("cloud %s.", quickly_dissipates);
*/
                    pline("�ق����%s�D",quickly_dissipates);
                return FALSE;
            }
            block_point(x, y);
            door->typ = SDOOR;
            if (vis)
/*JP
                pline_The("doorway vanishes!");
*/
                pline("�o�����͏������I");
            newsym(x, y);
            return TRUE;
        }
        if (obstructed(x, y, mysterywand))
            return FALSE;
        /* Don't allow doors to close over traps.  This is for pits */
        /* & trap doors, but is it ever OK for anything else? */
        if (t_at(x, y)) {
            /* maketrap() clears doormask, so it should be NODOOR */
#if 0 /*JP*/
            pline("%s springs up in the doorway, but %s.", dustcloud,
                  quickly_dissipates);
#else
            pline("%s���o�����ɗ������߂��C������%s", dustcloud,
                  quickly_dissipates);
#endif
            return FALSE;
        }

        switch (door->doormask & ~D_TRAPPED) {
        case D_CLOSED:
/*JP
            msg = "The door locks!";
*/
                msg = "���Ɍ������������I";
            break;
        case D_ISOPEN:
/*JP
            msg = "The door swings shut, and locks!";
*/
            msg = "���͐����悭�܂�C�������������I";
            break;
        case D_BROKEN:
/*JP
            msg = "The broken door reassembles and locks!";
*/
            msg = "��ꂽ�����č\������C�������������I";
            break;
        case D_NODOOR:
            msg =
/*JP
               "A cloud of dust springs up and assembles itself into a door!";
*/
                "�ق��肪�������߁C�W�܂��Ĕ��ɂȂ����I";
            break;
        default:
            res = FALSE;
            break;
        }
        block_point(x, y);
        door->doormask = D_LOCKED | (door->doormask & D_TRAPPED);
        newsym(x, y);
        break;
    case WAN_OPENING:
    case SPE_KNOCK:
        if (door->doormask & D_LOCKED) {
/*JP
            msg = "The door unlocks!";
*/
            msg = "���̌��͂͂��ꂽ�I";
            door->doormask = D_CLOSED | (door->doormask & D_TRAPPED);
        } else
            res = FALSE;
        break;
    case WAN_STRIKING:
    case SPE_FORCE_BOLT:
        if (door->doormask & (D_LOCKED | D_CLOSED)) {
            if (door->doormask & D_TRAPPED) {
                if (MON_AT(x, y))
                    (void) mb_trapped(m_at(x, y));
                else if (flags.verbose) {
                    if (cansee(x, y))
/*JP
                        pline("KABOOM!!  You see a door explode.");
*/
                        pline("����ǁ[��I�������������D");
                    else
/*JP
                        You_hear("a distant explosion.");
*/
                        You_hear("�����̔������𕷂����D");
                }
                door->doormask = D_NODOOR;
                unblock_point(x, y);
                newsym(x, y);
                loudness = 40;
                break;
            }
            door->doormask = D_BROKEN;
            if (flags.verbose) {
                if (cansee(x, y))
/*JP
                    pline_The("door crashes open!");
*/
                    pline("���͉��J�����I");
                else
/*JP
                    You_hear("a crashing sound.");
*/
                    You_hear("���������鉹�𕷂����D");
            }
            unblock_point(x, y);
            newsym(x, y);
            /* force vision recalc before printing more messages */
            if (vision_full_recalc)
                vision_recalc(0);
            loudness = 20;
        } else
            res = FALSE;
        break;
    default:
        impossible("magic (%d) attempted on door.", otmp->otyp);
        break;
    }
    if (msg && cansee(x, y))
        pline1(msg);
    if (loudness > 0) {
        /* door was destroyed */
        wake_nearto(x, y, loudness);
        if (*in_rooms(x, y, SHOPBASE))
            add_damage(x, y, 0L);
    }

    if (res && picking_at(x, y)) {
        /* maybe unseen monster zaps door you're unlocking */
        stop_occupation();
        reset_pick();
    }
    return res;
}

STATIC_OVL void
chest_shatter_msg(otmp)
struct obj *otmp;
{
    const char *disposition;
    const char *thing;
    long save_Blinded;

    if (otmp->oclass == POTION_CLASS) {
#if 0 /*JP*/
        You("%s %s shatter!", Blind ? "hear" : "see", an(bottlename()));
#else
        if (Blind)
            You_hear("%s������鉹�𕷂����I", bottlename());
        else
            pline("%s�����ꂽ�I", bottlename());
#endif
        if (!breathless(youmonst.data) || haseyes(youmonst.data))
            potionbreathe(otmp);
        return;
    }
    /* We have functions for distant and singular names, but not one */
    /* which does _both_... */
    save_Blinded = Blinded;
    Blinded = 1;
    thing = singular(otmp, xname);
    Blinded = save_Blinded;
    switch (objects[otmp->otyp].oc_material) {
    case PAPER:
/*JP
        disposition = "is torn to shreds";
*/
        disposition = "�͐��f���ꂽ";
        break;
    case WAX:
/*JP
        disposition = "is crushed";
*/
        disposition = "�����ɂԂ��܂���";
        break;
    case VEGGY:
/*JP
        disposition = "is pulped";
*/
        disposition = "�͂ǂ�ǂ�ɂȂ���";
        break;
    case FLESH:
/*JP
        disposition = "is mashed";
*/
        disposition = "�͂ǂ�ǂ�ɂȂ���";
        break;
    case GLASS:
/*JP
        disposition = "shatters";
*/
        disposition = "�͊��ꂽ";
        break;
    case WOOD:
/*JP
        disposition = "splinters to fragments";
*/
        disposition = "�͂�����ɂȂ���";
        break;
    default:
/*JP
        disposition = "is destroyed";
*/
        disposition = "�͉�ꂽ";
        break;
    }
/*JP
    pline("%s %s!", An(thing), disposition);
*/
    pline("%s%s�I", thing, disposition);
}

/*lock.c*/
