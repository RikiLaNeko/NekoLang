
# Interpréteur NekoLang

## Introduction

NekoLang est un langage de programmation simple et interprété, inspiré par la nature enjouée des chats. Il est conçu pour être facile à apprendre et amusant à utiliser. Le langage inclut des commandes comme `purr`, `kitten` et `meow` pour effectuer des tâches de programmation de base telles que l'affichage de messages, la déclaration de variables et la réception d'entrées de l'utilisateur.

Ce fichier README vous guidera dans la configuration de l'interpréteur NekoLang, l'écriture de code NekoLang et l'exécution de vos programmes.

## Table des matières

- [Prérequis](#prérequis)
- [Configuration](#configuration)
- [Écrire du code NekoLang](#écrire-du-code-nekolang)
    - [Structure du programme](#structure-du-programme)
    - [Commandes](#commandes)
- [Exemples](#exemples)
    - [Hello World](#hello-world)
    - [Variables](#variables)
    - [Entrée utilisateur](#entrée-utilisateur)
- [Exécution de programmes NekoLang](#exécution-de-programmes-nekolang)
- [Étendre NekoLang](#étendre-nekolang)
- [Contact](#contact)

## Prérequis

- Un compilateur C (par exemple, `gcc`)
- Connaissances de base de l'utilisation de la ligne de commande

## Configuration

### 1. Télécharger l'Interpréteur

Enregistrez le fichier `neko.c` (fourni ci-dessous) dans un répertoire de votre choix.

### 2. Compiler l'Interpréteur

Ouvrez votre terminal et naviguez jusqu'au répertoire contenant `neko.c`. Compilez l'interpréteur en utilisant la commande suivante :

```bash
gcc -o neko neko.c
```

Cette commande créera un exécutable nommé `neko`.

## Écrire du code NekoLang

### Structure du programme

Un programme NekoLang est encadré par les blocs `neko {` et `}`. Toutes les commandes doivent être placées à l'intérieur de ce bloc.

```plaintext
neko {
    // Votre code ici
}
```

### Commandes

- `purr` : Affiche un message dans la console.

  **Syntaxe** :

    ```plaintext
    purr "Votre message";
    ```

  Vous pouvez aussi concaténer des variables et des chaînes de caractères en utilisant l'opérateur `+` :

    ```plaintext
    purr "Bonjour, " + nom + "!";
    ```

- `kitten` : Déclare une variable et lui assigne une valeur.

  **Syntaxe** :

    ```plaintext
    kitten nomVariable = "valeur";
    ```

  Les variables peuvent également stocker des chaînes sans guillemets.

- `meow` : Demande une entrée à l'utilisateur et la stocke dans une variable.

  **Syntaxe** :

    ```plaintext
    meow nomVariable;
    ```

## Exemples

### Hello World

**Fichier : hello.neko**

```plaintext
neko {
    purr "Bonjour de NekoLang!";
}
```

**Explication** :  
La commande `purr` affiche "Bonjour de NekoLang!" dans la console.

### Variables

**Fichier : variables.neko**

```plaintext
neko {
    kitten nom = "Moustache";
    purr "Le nom du chat est " + nom + ".";
}
```

**Explication** :
- `kitten` déclare une variable `nom` avec la valeur "Moustache".
- `purr` affiche "Le nom du chat est Moustache." en concaténant des chaînes et la variable `nom`.

### Entrée utilisateur

**Fichier : input.neko**

```plaintext
neko {
    purr "Quel est ton plat préféré ?";
    meow plat;
    purr "J'aime aussi " + plat + "!";
}
```

**Explication** :
- `purr` affiche une question demandant à l'utilisateur son plat préféré.
- `meow` lit l'entrée de l'utilisateur et la stocke dans la variable `plat`.
- Le dernier `purr` affiche un message incluant la réponse de l'utilisateur.

## Exécution de programmes NekoLang

Pour exécuter un programme NekoLang, utilisez l'interpréteur suivi du nom du fichier :

```bash
./neko votreProgramme.neko
```

Exemple :

```bash
./neko hello.neko
```

**Sortie attendue** :

```plaintext
Bonjour de NekoLang!
```

## Étendre NekoLang

Vous pouvez étendre l'interpréteur en ajoutant de nouvelles commandes ou fonctionnalités à `neko.c`. N'hésitez pas à expérimenter et à ajouter vos propres fonctionnalités inspirées des chats !

## Contact

Pour toute question ou suggestion, n'hésitez pas à nous contacter à : [Vos informations de contact].
