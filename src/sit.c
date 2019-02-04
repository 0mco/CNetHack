/* NetHack 3.6	sit.c	$NHDT-Date: 1458341129 2016/03/18 22:45:29 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.53 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2012. */
/* NetHack may be freely redistributed.  See license for details. */

/* JNetHack Copyright */
/* (c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994-2000  */
/* For 3.4-, Copyright (c) SHIRAKATA Kentaro, 2002-2018            */
/* JNetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "artifact.h"


/* take away the hero's money */
void
take_gold()
{
    struct obj *otmp, *nobj;
    int lost_money = 0;

    for (otmp = invent; otmp; otmp = nobj) {
        nobj = otmp->nobj;
        if (otmp->oclass == COIN_CLASS) {
            lost_money = 1;
            remove_worn_item(otmp, FALSE);
            delobj(otmp);
        }
    }
    if (!lost_money) {
/*JP
        You_feel("a strange sensation.");
*/
        You("��Ȋ��o���o�����D");
    } else {
/*JP
        You("notice you have no money!");
*/
        You("�����������ĂȂ����ƂɋC�������I");
        context.botl = 1;
    }
}

/* #sit command */
int
dosit()
{
/*JP
    static const char sit_message[] = "sit on the %s.";
*/
    static const char sit_message[] = "%s�ɍ������D";
    register struct trap *trap = t_at(u.ux, u.uy);
    register int typ = levl[u.ux][u.uy].typ;

    if (u.usteed) {
/*JP
        You("are already sitting on %s.", mon_nam(u.usteed));
*/
        You("����%s�ɍ����Ă���D", mon_nam(u.usteed));
        return 0;
    }
    if (u.uundetected && is_hider(youmonst.data) && u.umonnum != PM_TRAPPER)
        u.uundetected = 0; /* no longer on the ceiling */

    if (!can_reach_floor(FALSE)) {
        if (u.uswallow)
/*JP
            There("are no seats in here!");
*/
            pline("�����ɂ͈֎q���Ȃ��I");
        else if (Levitation)
/*JP
            You("tumble in place.");
*/
            You("���̏�Œ��Ԃ肵���D");
        else
/*JP
            You("are sitting on air.");
*/
            You("�󒆂ɍ������D");
        return 0;
    } else if (u.ustuck && !sticks(youmonst.data)) {
        /* holding monster is next to hero rather than beneath, but
           hero is in no condition to actually sit at has/her own spot */
        if (humanoid(u.ustuck->data))
/*JP
            pline("%s won't offer %s lap.", Monnam(u.ustuck), mhis(u.ustuck));
*/
            pline("%s�͂Ђ����o���Ȃ������D", Monnam(u.ustuck));
        else
/*JP
            pline("%s has no lap.", Monnam(u.ustuck));
*/
            pline("%s�ɂ͂Ђ����Ȃ��D", Monnam(u.ustuck));
        return 0;
    } else if (is_pool(u.ux, u.uy) && !Underwater) { /* water walking */
        goto in_water;
    }

    if (OBJ_AT(u.ux, u.uy)
        /* ensure we're not standing on the precipice */
        && !uteetering_at_seen_pit(trap)) {
        register struct obj *obj;

        obj = level.objects[u.ux][u.uy];
        if (youmonst.data->mlet == S_DRAGON && obj->oclass == COIN_CLASS) {
#if 0 /*JP*/
            You("coil up around your %shoard.",
                (obj->quan + money_cnt(invent) < u.ulevel * 1000) ? "meager "
                                                                  : "");
#else
            You("%s����̂܂��łƂ�����������D",
                (obj->quan + money_cnt(invent) < u.ulevel * 1000) ? "�킸����"
                                                                  : "");
#endif
        } else {
/*JP
            You("sit on %s.", the(xname(obj)));
*/
            You("%s�ɍ������D", the(xname(obj)));
            if (!(Is_box(obj) || objects[obj->otyp].oc_material == CLOTH))
/*JP
                pline("It's not very comfortable...");
*/
                pline("���܂���育�������悭�Ȃ��D�D�D");
        }
    } else if (trap != 0 || (u.utrap && (u.utraptype >= TT_LAVA))) {
        if (u.utrap) {
            exercise(A_WIS, FALSE); /* you're getting stuck longer */
            if (u.utraptype == TT_BEARTRAP) {
/*JP
                You_cant("sit down with your %s in the bear trap.",
*/
                pline("%s���F��㩂ɂ͂��܂��Ă���̂ō���Ȃ��D",
                         body_part(FOOT));
                u.utrap++;
            } else if (u.utraptype == TT_PIT) {
                if (trap && trap->ttyp == SPIKED_PIT) {
/*JP
                    You("sit down on a spike.  Ouch!");
*/
                    You("�g�Q�̏�ɍ������D���Ă��I");
                    losehp(Half_physical_damage ? rn2(2) : 1,
/*JP
                           "sitting on an iron spike", KILLED_BY);
*/
                           "�S�̃g�Q�̏�ɍ�����", KILLED_BY);
                    exercise(A_STR, FALSE);
                } else
/*JP
                    You("sit down in the pit.");
*/
                    You("�������̒��ō������D");
                u.utrap += rn2(5);
            } else if (u.utraptype == TT_WEB) {
/*JP
                You("sit in the spider web and get entangled further!");
*/
                You("�����̑��̒��ō�������C�܂��܂����܂����I");
                u.utrap += rn1(10, 5);
            } else if (u.utraptype == TT_LAVA) {
                /* Must have fire resistance or they'd be dead already */
/*JP
                You("sit in the %s!", hliquid("lava"));
*/
                You("%s�̒��ɍ������I", hliquid("�n��"));
                if (Slimed)
                    burn_away_slime();
                u.utrap += rnd(4);
/*JP
                losehp(d(2, 10), "sitting in lava",
*/
                losehp(d(2, 10), "�n��̒��ɍ�����",
                       KILLED_BY); /* lava damage */
            } else if (u.utraptype == TT_INFLOOR
                       || u.utraptype == TT_BURIEDBALL) {
/*JP
                You_cant("maneuver to sit!");
*/
                You("����悤�ȓ��삪�ł��Ȃ��I");
                u.utrap++;
            }
        } else {
/*JP
            You("sit down.");
*/
            You("�������D");
            dotrap(trap, VIASITTING);
        }
    } else if (Underwater || Is_waterlevel(&u.uz)) {
        if (Is_waterlevel(&u.uz))
/*JP
            There("are no cushions floating nearby.");
*/
            pline("�߂��ɕ����Ă���N�b�V�����͂Ȃ��D");
        else
/*JP
            You("sit down on the muddy bottom.");
*/
            You("�ǂ�ǂ낵����ɍ������D");
    } else if (is_pool(u.ux, u.uy)) {
    in_water:
/*JP
        You("sit in the %s.", hliquid("water"));
*/
        You("%s�̒��ō������D", hliquid("��"));
        if (!rn2(10) && uarm)
/*JP
            (void) water_damage(uarm, "armor", TRUE);
*/
            (void) water_damage(uarm, "�Z", TRUE);
        if (!rn2(10) && uarmf && uarmf->otyp != WATER_WALKING_BOOTS)
/*JP
            (void) water_damage(uarm, "armor", TRUE);
*/
            (void) water_damage(uarm, "�Z", TRUE);
    } else if (IS_SINK(typ)) {
        You(sit_message, defsyms[S_sink].explanation);
/*JP
        Your("%s gets wet.", humanoid(youmonst.data) ? "rump" : "underside");
*/
        Your("%s�͔G�ꂽ�D", humanoid(youmonst.data) ? "�K" : "����");
    } else if (IS_ALTAR(typ)) {
        You(sit_message, defsyms[S_altar].explanation);
        altar_wrath(u.ux, u.uy);
    } else if (IS_GRAVE(typ)) {
        You(sit_message, defsyms[S_grave].explanation);
    } else if (typ == STAIRS) {
/*JP
        You(sit_message, "stairs");
*/
        You(sit_message, "�K�i");
    } else if (typ == LADDER) {
/*JP
        You(sit_message, "ladder");
*/
        You(sit_message, "�͂���");
    } else if (is_lava(u.ux, u.uy)) {
        /* must be WWalking */
/*JP
        You(sit_message, hliquid("lava"));
*/
        You(sit_message, hliquid("�n��"));
        burn_away_slime();
        if (likes_lava(youmonst.data)) {
/*JP
            pline_The("%s feels warm.", hliquid("lava"));
*/
            pline_The("%s�͒g�����D", hliquid("�n��"));
            return 1;
        }
/*JP
        pline_The("%s burns you!", hliquid("lava"));
*/
        pline_The("%s�ŔR�����I", hliquid("�n��"));
        losehp(d((Fire_resistance ? 2 : 10), 10), /* lava damage */
/*JP
               "sitting on lava", KILLED_BY);
*/
               "�n��ɍ�����", KILLED_BY);
    } else if (is_ice(u.ux, u.uy)) {
        You(sit_message, defsyms[S_ice].explanation);
        if (!Cold_resistance)
/*JP
            pline_The("ice feels cold.");
*/
            pline("�X�͗₽���������D");
    } else if (typ == DRAWBRIDGE_DOWN) {
/*JP
        You(sit_message, "drawbridge");
*/
        You(sit_message, "���ˋ�");
    } else if (IS_THRONE(typ)) {
        You(sit_message, defsyms[S_throne].explanation);
        if (rnd(6) > 4) {
            switch (rnd(13)) {
            case 1:
                (void) adjattrib(rn2(A_MAX), -rn1(4, 3), FALSE);
/*JP
                losehp(rnd(10), "cursed throne", KILLED_BY_AN);
*/
                losehp(rnd(10), "���ꂽ�ʍ���", KILLED_BY_AN);
                break;
            case 2:
                (void) adjattrib(rn2(A_MAX), 1, FALSE);
                break;
            case 3:
#if 0 /*JP*/
                pline("A%s electric shock shoots through your body!",
                      (Shock_resistance) ? "n" : " massive");
#else
                pline("%s�d�C�����Ȃ��̑̂𑖂蔲�����I",
                      (Shock_resistance) ? "" : "������");
#endif
/*JP
                losehp(Shock_resistance ? rnd(6) : rnd(30), "electric chair",
*/
                losehp(Shock_resistance ? rnd(6) : rnd(30), "�d�C�֎q��",
                       KILLED_BY_AN);
                exercise(A_CON, FALSE);
                break;
            case 4:
/*JP
                You_feel("much, much better!");
*/
                You_feel("�ƂĂ��C�ƂĂ����C�ɂȂ����悤�ȋC�������I");
                if (Upolyd) {
                    if (u.mh >= (u.mhmax - 5))
                        u.mhmax += 4;
                    u.mh = u.mhmax;
                }
                if (u.uhp >= (u.uhpmax - 5))
                    u.uhpmax += 4;
                u.uhp = u.uhpmax;
                u.ucreamed = 0;
                make_blinded(0L, TRUE);
                make_sick(0L, (char *) 0, FALSE, SICK_ALL);
                heal_legs();
                context.botl = 1;
                break;
            case 5:
                take_gold();
                break;
            case 6:
                if (u.uluck + rn2(5) < 0) {
/*JP
                    You_feel("your luck is changing.");
*/
                    pline("�^�������Ă����C������D");
                    change_luck(1);
                } else
                    makewish();
                break;
            case 7:
              {
                int cnt = rnd(10);

                /* Magical voice not affected by deafness */
/*JP
                pline("A voice echoes:");
*/
                pline("����������:");
#if 0 /*JP*/
                verbalize("Thy audience hath been summoned, %s!",
                          flags.female ? "Dame" : "Sire");
#else
                verbalize("%s��I���̒��O�������ꂵ�D",
                          flags.female ? "��" : "�j");
#endif
                while (cnt--)
                    (void) makemon(courtmon(), u.ux, u.uy, NO_MM_FLAGS);
                break;
              }
            case 8:
                /* Magical voice not affected by deafness */
/*JP
                pline("A voice echoes:");
*/
                pline("����������:");
#if 0 /*JP*/
                verbalize("By thine Imperious order, %s...",
                          flags.female ? "Dame" : "Sire");
#else
                verbalize("%s��I���̘�����������悤���D",
                          flags.female ? "��" : "�j");
#endif
                do_genocide(5); /* REALLY|ONTHRONE, see do_genocide() */
                break;
            case 9:
                /* Magical voice not affected by deafness */
/*JP
                pline("A voice echoes:");
*/
                pline("����������:");
                verbalize(
/*JP
                 "A curse upon thee for sitting upon this most holy throne!");
*/
                 "���Ȃ�ʍ��ɍ��肵���Ɏ􂢂���I");
                if (Luck > 0) {
                    make_blinded(Blinded + rn1(100, 250), TRUE);
                    change_luck((Luck > 1) ? -rnd(2) : -1);
                } else
                    rndcurse();
                break;
            case 10:
                if (Luck < 0 || (HSee_invisible & INTRINSIC)) {
                    if (level.flags.nommap) {
/*JP
                        pline("A terrible drone fills your head!");
*/
                        pline("�������u���u���Ƃ����������ɋ������I");
                        make_confused((HConfusion & TIMEOUT) + (long) rnd(30),
                                      FALSE);
                    } else {
/*JP
                        pline("An image forms in your mind.");
*/
                        pline("����C���[�W�����ɕ��񂾁D");
                        do_mapping();
                    }
                } else {
/*JP
                    Your("vision becomes clear.");
*/
                    Your("���E�͍Ⴆ�n�����D");
                    HSee_invisible |= FROMOUTSIDE;
                    newsym(u.ux, u.uy);
                }
                break;
            case 11:
                if (Luck < 0) {
/*JP
                    You_feel("threatened.");
*/
                    You("��������Ă���悤�ȋC�������D");
                    aggravate();
                } else {
/*JP
                    You_feel("a wrenching sensation.");
*/
                    You("�˂���ꂽ�悤�Ȋ��o���������D");
                    tele(); /* teleport him */
                }
                break;
            case 12:
/*JP
                You("are granted an insight!");
*/
                You("���@�͂𓾂��I");
                if (invent) {
                    /* rn2(5) agrees w/seffects() */
                    identify_pack(rn2(5), FALSE);
                }
                break;
            case 13:
/*JP
                Your("mind turns into a pretzel!");
*/
                Your("�S�̓N�l�N�l�ɂȂ����I");
                make_confused((HConfusion & TIMEOUT) + (long) rn1(7, 16),
                              FALSE);
                break;
            default:
                impossible("throne effect");
                break;
            }
        } else {
            if (is_prince(youmonst.data))
/*JP
                You_feel("very comfortable here.");
*/
                You("�����͂ƂĂ����������D");
            else
/*JP
                You_feel("somehow out of place...");
*/
                You("������Ⴂ�̋C�������D�D�D");
        }

        if (!rn2(3) && IS_THRONE(levl[u.ux][u.uy].typ)) {
            /* may have teleported */
            levl[u.ux][u.uy].typ = ROOM;
/*JP
            pline_The("throne vanishes in a puff of logic.");
*/
            pline("�ʍ��͂ӂ��Ə������D");
            newsym(u.ux, u.uy);
        }
    } else if (lays_eggs(youmonst.data)) {
        struct obj *uegg;

        if (!flags.female) {
#if 0 /*JP*/
            pline("%s can't lay eggs!",
                  Hallucination
                      ? "You may think you are a platypus, but a male still"
                      : "Males");
#else
            pline("%s�Y�͗����Y�߂Ȃ��I",
                  Hallucination
                      ? "���Ȃ��͎������J���m�n�V���Ǝv���Ă��邩������Ȃ����C����ς�"
                      : "");
#endif
            return 0;
        } else if (u.uhunger < (int) objects[EGG].oc_nutrition) {
/*JP
            You("don't have enough energy to lay an egg.");
*/
            You("�����Y�ނ����̃G�l���M�[���Ȃ��D");
            return 0;
        }

        uegg = mksobj(EGG, FALSE, FALSE);
        uegg->spe = 1;
        uegg->quan = 1L;
        uegg->owt = weight(uegg);
        /* this sets hatch timers if appropriate */
        set_corpsenm(uegg, egg_type_from_parent(u.umonnum, FALSE));
        uegg->known = uegg->dknown = 1;
/*JP
        You("lay an egg.");
*/
        You("�����Y�񂾁D");
        dropy(uegg);
        stackobj(uegg);
        morehungry((int) objects[EGG].oc_nutrition);
    } else {
/*JP
        pline("Having fun sitting on the %s?", surface(u.ux, u.uy));
*/
        pline("%s�ɍ����Ċy���������H", surface(u.ux,u.uy));
    }
    return 1;
}

/* curse a few inventory items at random! */
void
rndcurse()
{
    int nobj = 0;
    int cnt, onum;
    struct obj *otmp;
/*JP
    static const char mal_aura[] = "feel a malignant aura surround %s.";
*/
    static const char mal_aura[] = "�׈��ȃI�[����%s�̉��Ɋ������D";

    if (uwep && (uwep->oartifact == ART_MAGICBANE) && rn2(20)) {
/*JP
        You(mal_aura, "the magic-absorbing blade");
*/
        You(mal_aura, "���͂��z���Ƃ铁");
        return;
    }

    if (Antimagic) {
        shieldeff(u.ux, u.uy);
/*JP
        You(mal_aura, "you");
*/
        You(mal_aura, "���Ȃ�");
    }

    for (otmp = invent; otmp; otmp = otmp->nobj) {
        /* gold isn't subject to being cursed or blessed */
        if (otmp->oclass == COIN_CLASS)
            continue;
        nobj++;
    }
    if (nobj) {
        for (cnt = rnd(6 / ((!!Antimagic) + (!!Half_spell_damage) + 1));
             cnt > 0; cnt--) {
            onum = rnd(nobj);
            for (otmp = invent; otmp; otmp = otmp->nobj) {
                /* as above */
                if (otmp->oclass == COIN_CLASS)
                    continue;
                if (--onum == 0)
                    break; /* found the target */
            }
            /* the !otmp case should never happen; picking an already
               cursed item happens--avoid "resists" message in that case */
            if (!otmp || otmp->cursed)
                continue; /* next target */

            if (otmp->oartifact && spec_ability(otmp, SPFX_INTEL)
                && rn2(10) < 8) {
/*JP
                pline("%s!", Tobjnam(otmp, "resist"));
*/
                pline("%s�͉e�����󂯂Ȃ��I", xname(otmp));
                continue;
            }

            if (otmp->blessed)
                unbless(otmp);
            else
                curse(otmp);
        }
        update_inventory();
    }

    /* treat steed's saddle as extended part of hero's inventory */
    if (u.usteed && !rn2(4) && (otmp = which_armor(u.usteed, W_SADDLE)) != 0
        && !otmp->cursed) { /* skip if already cursed */
        if (otmp->blessed)
            unbless(otmp);
        else
            curse(otmp);
        if (!Blind) {
#if 0 /*JP*/
            pline("%s %s.", Yobjnam2(otmp, "glow"),
                  hcolor(otmp->cursed ? NH_BLACK : (const char *) "brown"));
#else
            pline("%s��%s�P�����D", xname(otmp),
                  jconj_adj(hcolor(otmp->cursed ? NH_BLACK : (const char *)"���F��")));
#endif
            otmp->bknown = TRUE;
        }
    }
}

/* remove a random INTRINSIC ability */
void
attrcurse()
{
    switch (rnd(11)) {
    case 1:
        if (HFire_resistance & INTRINSIC) {
            HFire_resistance &= ~INTRINSIC;
/*JP
            You_feel("warmer.");
*/
            You("�g�������������D");
            break;
        }
        /*FALLTHRU*/
    case 2:
        if (HTeleportation & INTRINSIC) {
            HTeleportation &= ~INTRINSIC;
/*JP
            You_feel("less jumpy.");
*/
            You("������Ɨ��������D");
            break;
        }
        /*FALLTHRU*/
    case 3:
        if (HPoison_resistance & INTRINSIC) {
            HPoison_resistance &= ~INTRINSIC;
/*JP
            You_feel("a little sick!");
*/
            You("�����C���������Ȃ����I");
            break;
        }
        /*FALLTHRU*/
    case 4:
        if (HTelepat & INTRINSIC) {
            HTelepat &= ~INTRINSIC;
            if (Blind && !Blind_telepat)
                see_monsters(); /* Can't sense mons anymore! */
/*JP
            Your("senses fail!");
*/
            Your("�܊��͖�Ⴢ����I");
            break;
        }
        /*FALLTHRU*/
    case 5:
        if (HCold_resistance & INTRINSIC) {
            HCold_resistance &= ~INTRINSIC;
/*JP
            You_feel("cooler.");
*/
            You("���������������D");
            break;
        }
        /*FALLTHRU*/
    case 6:
        if (HInvis & INTRINSIC) {
            HInvis &= ~INTRINSIC;
/*JP
            You_feel("paranoid.");
*/
            You("�ϑz��������D");
            break;
        }
        /*FALLTHRU*/
    case 7:
        if (HSee_invisible & INTRINSIC) {
            HSee_invisible &= ~INTRINSIC;
#if 0 /*JP*/
            You("%s!", Hallucination ? "tawt you taw a puttie tat"
                                     : "thought you saw something");
#else
            if(Hallucination)
                You("����I�݂�C��Ă���D");
            else
                You("�N���Ɍ����Ă���悤�ȋC�������I");
#endif
            break;
        }
        /*FALLTHRU*/
    case 8:
        if (HFast & INTRINSIC) {
            HFast &= ~INTRINSIC;
/*JP
            You_feel("slower.");
*/
            You("�x���Ȃ����悤�ȋC�������D");
            break;
        }
        /*FALLTHRU*/
    case 9:
        if (HStealth & INTRINSIC) {
            HStealth &= ~INTRINSIC;
/*JP
            You_feel("clumsy.");
*/
            You("�s��p�ɂȂ����悤�ȋC�������D");
            break;
        }
        /*FALLTHRU*/
    case 10:
        /* intrinsic protection is just disabled, not set back to 0 */
        if (HProtection & INTRINSIC) {
            HProtection &= ~INTRINSIC;
/*JP
            You_feel("vulnerable.");
*/
            You("���h���ɂȂ����C�������D");
            break;
        }
        /*FALLTHRU*/
    case 11:
        if (HAggravate_monster & INTRINSIC) {
            HAggravate_monster &= ~INTRINSIC;
/*JP
            You_feel("less attractive.");
*/
            You("���͂��������悤�ȋC�������D");
            break;
        }
        /*FALLTHRU*/
    default:
        break;
    }
}

/*sit.c*/
