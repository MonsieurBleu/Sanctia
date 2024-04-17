# Fariah Grisnier

## TALK

##### INTERLOCUTOR

>INTERLOCUTOR_NEMESIS

[fr- Tu es mon pire enemi, je te hais de tout mon coeur]
= ~

>INTERLOCUTOR_HOSTILE

[fr- Je suis hostile envers toi]
= ~

>INTERLOCUTOR_DESPISED

[fr- Je te méprise fort mon reuf]
= ~

>INTERLOCUTOR_BAD_ACQUAINTANCE

[fr- Tu ne m'a pas l'air très sympa toi...]
= ~

>INTERLOCUTOR_MAX_AFFINITY

[fr- Epouse moi ! Notre enfant s'appelera Cédric et on l'aimera fort malgré son nom de merde]
= ~

>INTERLOCUTOR_TRUSTED

[fr- Je te ferais confiance les yeux fermé toi, toi lo lo t'es le sang du sang de la mama]
= ~

>INTERLOCUTOR_FRIENDLY

[fr- Yooo le sangue comment ça va !?]
= ~

>INTERLOCUTOR_GOOD_ACQUAINTANCE

[fr- T'as l'air {d'une meuf}{d'un gars} cool toi]
= ~


>!INTERLOCUTOR_KNOWN

[fr- Qu'est-ce {qu'une jeune femme}{qu'un jeune homme} sans défense vient faire ici ?]
[en- What is a defensless {woman}{man} like you doing here ?]
= ~

>INTERLOCUTOR_KNOWN
>COND_RANDOM

[fr- Encore vous ?]
[en- You, again ?]
= ~

>INTERLOCUTOR_KNOWN
>COND_RANDOM

[fr- Dites-moi...]
[en- I'm hearing you...]
= ~

##### PLAYER

>!INTERLOCUTOR_KNOWN

[fr_ Parlez-moi de vous.]
[en_ What about you ?]
= @@@PRESENTATION

>COND_ALL

[fr_ *Un chiffre aléatoire entre 1 et 5.*]
= @@@RANDOM_TEST

>COND_ALL

[fr_ Savez vous où se situe la *boulangerie* le **Joyeux luron**]
= @BOULANGERIE

>COND_ALL

[fr_ T'es mon pote mtn ^^]
= EVENT_APPRECIATION_UP

>COND_ALL

[fr_ Je te trouve moche enfaite :(]
= EVENT_APPRECIATION_DOWN

## PRESENTATION

##### INTERLOCUTOR

>!INTERLOCUTOR_KNOWN

[fr- Bonjour, je suis skibidi]
= +INTERLOCUTOR_KNOWN
= @@@TALK

>INTERLOCUTOR_KNOWN

[fr_ *Vous* êtes le fameux **skibidi**]
= @@@PRESENTATION_IN_DEPTH

## PRESENTATION_IN_DEPTH

##### INTERLOCUTOR

>INTERLOCUTOR_KNOWN

[fr- Effectivement, je le fus, mais j'ai reçus une flêche dans le genou il a *trois* (**3** ***(pour de vrai)***) secondes.]
= ~

##### PLAYER

>INTERLOCUTOR_KNOWN

[fr_ Ok, mais je m'en fou...]
= @@TALK

## RANDOM_TEST

##### INTERLOCUTOR

>COND_RANDOM

[fr- 1]
= ~

>COND_RANDOM

[fr- 2]
= ~

>COND_RANDOM

[fr- 3]
= ~

>COND_RANDOM

[fr- 4]
= ~

>COND_RANDOM

[fr- 5]
= ~

##### PLAYER

>COND_ALL

[fr_ ***Encore !!!!!***]
= @@@RANDOM_TEST

>COND_ALL

[fr_ J'en ait marre, on parlait de quoi déjà ?]
= @@TALK

## BOULANGERIE

##### INTERLOCUTOR

>COND_ALL
>COND_RANDOM

[fr- Oui, au croisement entre la rue *Poggies* et l'avenue *Gentil Garçon*]
= ~

>COND_ALL
>COND_RANDOM

[fr- Oui, c'est moi]
= ~

##### PLAYER

>COND_ALL

[fr_ Merci, c'est bien utile de savoir ça]
= @@TALK

>COND_ALL

[fr_ Je m'en bas la noisette, cordiallement]
= @@TALK

