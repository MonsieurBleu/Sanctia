```md
| Signe  | Utilisation                                                                                |
|--------|--------------------------------------------------------------------------------------------|
| #      | Début de l'entrée d'un personnage                                                          |
| ##     | Début de l'entrée d'un dialogue du personnage                                              |
| >      | Prérequis, toujours suivis d'une condition, COND_ALL si aucun prérequis                    |
| >!     | Prérequis inversé                                                                          |
| []     | Contient le texte                                                                          |
| fr_    | Toujours après l'ouverture des [], détermine qu'il s'agit d'une réponse du PJ en français  |
| fr-    | Toujours après l'ouverture des [], détermine qu'il s'agit d'une réponse du PNJ en français |
| {F}{H} | F sera affiché si le joueur a un corps féminin, H sinon                                    |
| =      | tout le temps après la liste des textes entre [], nouvelle conséquence                     |
| = ~    | conséquence vide                                                                           |
| = +    | toujours suivi d'une condition, met la condition à vrai                                    |
| = -    | toujours suivi d'une condition, met la condition à faux                                    |
| = @    | toujours suivi d'un dialogue, change le dialogue actif par celui donné                     |
| = @@   | idem, mais vide la liste des réponses du joueurs                                           |
```

# Fariah Grisnier

## TALK

>!COND_NPC_KNOWN
[fr- Qu'est-ce {qu'une jeune femme}{qu'un jeune homme} sans défense vient faire ici ?]
[en- What is a defensless {woman}{man} like you doing here ?]
= +COND_NPC_KNOWN

>COND_NPC_KNOWN
>COND_RANDOM
[fr- Encore vous ?]
[en- You, again ?]
= ~

>COND_NPC_KNOWN
>COND_RANDOM
[fr- Dites-moi...]
[en- I'm hearing you...]
= ~

>COND_ALL
[fr_ Parlez-moi de vous.]
[en_ What about you ?]
= EVENT_APPRECIATION_UP
= @PRESENTATION

>COND_ALL
[fr_ Un chiffre aléatoire entre 1 et 5.]
= @@RANDOM_TEST

## PRESENTATION

>COND_ALL
[fr- Bonjour, je suis...]
= ~

## RANDOM_TEST

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

>COND_RANDOM
[fr_ Encore !]
= @@RANDOM_TEST