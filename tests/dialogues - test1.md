


Conditions
```
	ALL
	RANDOM
	FEMALE 
	HIGHT_EQUIPEMENT
	FARIAH_PLAYER_KNOW_RATPROBLEM
	FARIAH_NPC_PLAYER_KNOW_RATPROBLEM
	FARIAH_RAT_PLOBEM_SOLVED
```

Events
```
	APPRECIATION_DOWN
	APPRECIATION_UP
	NPC_KNOWN
	NPC_NAME_KNOWN
```

Synthax
```
#\  				for character unique name/id
##\					for categorie
###\ 				for unique name/id
> 					condition required for appending
[LANGUAGE]: 		appending on string output
[LANGUAGE]- 		appending on dialogue output
[LANGUAGE]_			appending on player choice output
-	 				(after a choice/dialogue) push new events when triggered
+                   (before a condiiton) set the condition to true 
-                   (before a condiiton) set the condition to false
@ 					(before a dialogue id) change the current dialogue displayed & add new dialogue choices
@@ 					(before a dialogue id) change the current dialogue & flush all choices
```



# FARIAH
## NAME
### FULL_NAME

>ALL

FR: Fariah Grisnier

## DIALOGUE
### TALK
>!NPC_KNOWN
>!HIGHT_EQUIPEMENT

FR- Qu'est-ce {qu'une jeune femme}{qu'un jeune homme} sans défense vient faire ici ?

>!NPC_KNOWN
>HIGHT_EQUIPEMENT

FR- Ma parole vous êtes {prête}{prêt} pour la guerre vous ! Faites attention avec ça.

>NPC_KNOWN

FR- Ah, encore vous... Qu'il y a t-il ? 

>NPC_KNOWN 
>FARIAH_NPC_PLAYER_KNOW_RATPROBLEM

FR- Bon, vous en avez fini avec ces vermines ? J'ai une *affaire* à faire tourner.

>ALL

FR_ Parlez-moi de vous. 
-	+NPC_KNOWN
-	@PRESENTATION 

>KNOW_RAT_PROBLEM

FR_ Je viens vous aidez pour vos problèmes de rats
- +NPC_KNOWN 
- +FARIAH_NPC_PLAYER_KNOW_RATPROBLEM
- APPRECIATION_UP
- @@RAT_PROBLEM

### PRESENTATION 
>!FARIAH_NPC_PLAYER_KNOW_RATPROBLEM

FR- A l'heure qu'il est je pourrait tout aussi bien être une mendiante. Trois semaines que je n'ai pas pus ouvrir boutique à cause **d'une infestation de rats**... Foutus vermines. Si vous n'avez pas peur de chopper un truc et que vous vous sentez {courageuse}{courageux} vous pouvez toujours y jeter un œil... Contre **récompense** bien-sûr.

>FARIAH_NPC_PLAYER_KNOW_RATPROBLEM
>!FARIAH_RAT_PLOBEM_SOLVED

FR- Je peut être une excellente amie et partenaire d'affaires si vous m'aidez.

>FARIAH_RAT_PLOBEM_SOLVED 

FR- Je m'appelle Fariah Grisnier, j'ai repris ce taudis il y a de cela une bonne poignée de printemps après que ma mère ait succombe à la maladie. *Pas de condoléances* s'il vous plait, c'était une vraie sorcière, mon plus grand plaisir a été de tout refaire à neuf ici... Enfin, l'enseigne n'a pas brillé très longtemps. Mais grâce à vous, je peu enfin regagner ma croute.
- +NPC_NAME_KNOWN

>FARIAH_RAT_PLOBEM_SOLVED 

FR_ C'était un plaisir d'avoir pus aider.
- APPRECIATION_UP
- APPRECIATION_UP

FR_ Votre vie est passionnante, mais j'essayais simplement de faire la discussion pour être {cordiale}{cordial}.
- APPRECIATION_DOWN
