; #######################################
; french.nsh
; french language strings for inkscape installer
; windows code page: 1252
; Authors:
; Adib Taraben theAdib@yahoo.com
; matiphas matiphas@free.fr
;
; 27 july 2006 new languages en_CA, en_GB, fi, hr, mn, ne, rw, sq
; 11 august 2006 new languages dz bg

!insertmacro MUI_LANGUAGE "French"

; Product name
LangString lng_Caption   ${LANG_FRENCH} "${PRODUCT_NAME} -- Editeur vectoriel SVG libre"

; Button text "Next >" for the license page
LangString lng_LICENSE_BUTTON   ${LANG_FRENCH} "Suivant >"

; Bottom text for the license page
LangString lng_LICENSE_BOTTOM_TEXT   ${LANG_FRENCH} "$(^Name) est diffus� sous la licence publique g�n�rale (GPL) GNU. La licence est fournie ici pour information uniquement. $_CLICK"

;has been installed by different user
LangString lng_DIFFERENT_USER ${LANG_FRENCH} "Inkscape a d�j� �t� install� par l'utilisateur $0.$\r$\nSi vous continuez, l'installation pourrait devenir d�fectueuse!$\r$\nVeuillez, svp, vous connecter en tant que $0 et essayer de nouveau."

;you have no admin rigths
LangString lng_NO_ADMIN ${LANG_FRENCH} "Vous n'avez pas les privil�ges d'administrateur.$\r$\nL'installation d'Inkscape pour tous les utilisateurs pourrait devenir d�fectueuse.$\r$\nVeuillez d�cocher l'option 'pour tous les utilisateurs'."

;win9x is not supported
LangString lng_NOT_SUPPORTED ${LANG_FRENCH} "Inkscape n'est pas �x�cutable sur Windows 95/98/ME!$\r$\nVeuillez, svp, consulter les site web officiel pour plus d'informations."

; Full install type
LangString lng_Full $(LANG_FRENCH) "Compl�te"

; Optimal install type
LangString lng_Optimal $(LANG_FRENCH) "Optimale"

; Minimal install type
LangString lng_Minimal $(LANG_FRENCH) "Minimale"

; Core install section
LangString lng_Core $(LANG_FRENCH) "Editeur SVG ${PRODUCT_NAME} (n�cessaire)"

; Core install section description
LangString lng_CoreDesc $(LANG_FRENCH) "Fichiers indispensables d'${PRODUCT_NAME} et dlls"

; GTK+ install section
LangString lng_GTKFiles $(LANG_FRENCH) "Environnement GTK+ (n�cessaire)"

; GTK+ install section description
LangString lng_GTKFilesDesc $(LANG_FRENCH) "Une bo�te � outils multi-plateformes pour interfaces graphiques,utilis�e par ${PRODUCT_NAME}"

; shortcuts install section
LangString lng_Shortcuts $(LANG_FRENCH) "Raccourcis"

; shortcuts install section description
LangString lng_ShortcutsDesc $(LANG_FRENCH) "Raccourcis pour d�marrer ${PRODUCT_NAME}"

; All user install section
LangString lng_Alluser $(LANG_FRENCH) "pour tous les utilisateurs"

; All user install section description
LangString lng_AlluserDesc $(LANG_FRENCH) "Installer cette application pour tous les utilisateurs de cet ordinateurs"

; Desktop section
LangString lng_Desktop $(LANG_FRENCH) "Bureau"

; Desktop section description
LangString lng_DesktopDesc $(LANG_FRENCH) "Cr�er un raccourci vers ${PRODUCT_NAME} sur le bureau"

; Start Menu  section
LangString lng_Startmenu $(LANG_FRENCH) "Menu d�marrer"

; Start Menu section description
LangString lng_StartmenuDesc $(LANG_FRENCH) "Cr�er une entr�e ${PRODUCT_NAME} dans le menu d�marrer"

; Quick launch section
LangString lng_Quicklaunch $(LANG_FRENCH) "Lancement rapide"

; Quick launch section description
LangString lng_QuicklaunchDesc $(LANG_FRENCH) "Cr�er un raccourci vers ${PRODUCT_NAME} dans la barre de lancement rapide"

; File type association for editing
LangString lng_SVGWriter ${LANG_FRENCH} "Ouvrir les fichiers SVG avec ${PRODUCT_NAME}"

; File type association for editing description
LangString lng_SVGWriterDesc ${LANG_FRENCH} "Choisir ${PRODUCT_NAME} comme �diteur par d�faut pour les fichiers SVG"

; Context Menu
LangString lng_ContextMenu ${LANG_FRENCH} "Menu contextuel"

; Context Menu description
LangString lng_ContextMenuDesc ${LANG_FRENCH} "Ajouter ${PRODUCT_NAME} dans le menu contextuel des fichiers SVG"


; Additional files section
LangString lng_Addfiles $(LANG_FRENCH) "Fichiers additionnels"

; Additional files section description
LangString lng_AddfilesDesc $(LANG_FRENCH) "Fichiers additionnels"

; Examples section
LangString lng_Examples $(LANG_FRENCH) "Exemples"

; Examples section description
LangString lng_ExamplesDesc $(LANG_FRENCH) "Examples d'utilisation d'${PRODUCT_NAME}"

; Tutorials section
LangString lng_Tutorials $(LANG_FRENCH) "Didacticiels"

; Tutorials section description
LangString lng_TutorialsDesc $(LANG_FRENCH) "Didacticiels sur l'utilisation d'${PRODUCT_NAME}"


; Languages section
LangString lng_Languages $(LANG_FRENCH) "Traductions"

; Languages section dscription
LangString lng_LanguagesDesc $(LANG_FRENCH) "Installer des traductions pour ${PRODUCT_NAME}"

LangString lng_am $(LANG_FRENCH) "am  Amharique"
LangString lng_az $(LANG_FRENCH) "az  Azerba�djanais"
LangString lng_be $(LANG_FRENCH) "be  Bi�lorusse"
LangString lng_bg $(LANG_FRENSH) "bg  Bulgarian"
LangString lng_ca $(LANG_FRENCH) "ca  Catalan"
LangString lng_cs $(LANG_FRENCH) "cs  Tch�que"
LangString lng_da $(LANG_FRENCH) "da  Danois"
LangString lng_de $(LANG_FRENCH) "de  Allemand"
LangString lng_dz $(LANG_FRENCH) "dz  Dzongkha"
LangString lng_el $(LANG_FRENCH) "el  Grec"
LangString lng_en $(LANG_FRENCH) "en  Anglais"
LangString lng_en_CA $(LANG_FRENCH) "en_CA Canadian English"
LangString lng_en_GB $(LANG_FRENCH) "en_GB British English"
LangString lng_es $(LANG_FRENCH) "es  Espagnol"
LangString lng_es_MX $(LANG_FRENCH) "es_MX  Espagnol Mexique"
LangString lng_et $(LANG_FRENCH) "es  Estonien"
LangString lng_fi $(LANG_FRENCH) "fi  Finish"
LangString lng_fr $(LANG_FRENCH) "fr  French"
LangString lng_ga $(LANG_FRENCH) "ga  Irlandais"
LangString lng_gl $(LANG_FRENCH) "gl  Galicien"
LangString lng_hr $(LANG_FRENCH) "hr  Croatian"
LangString lng_hu $(LANG_FRENCH) "hu  Hongrois"
LangString lng_it $(LANG_FRENCH) "it  Italien"
LangString lng_ja $(LANG_FRENCH) "ja  Japonais"
LangString lng_ko $(LANG_FRENCH) "ko  Cor�en"
LangString lng_lt $(LANG_FRENCH) "lt  Lituanien"
LangString lng_mk $(LANG_FRENCH) "mk  Mac�donien"
LangString lng_mn $(LANG_FRENCH) "mn  Mongolian"
LangString lng_ne $(LANG_FRENCH) "ne  Nepali"
LangString lng_nb $(LANG_FRENCH) "nb  Norv�gien Bokmal"
LangString lng_nl $(LANG_FRENCH) "nl  N�erlandais"
LangString lng_nn $(LANG_FRENCH) "nn  Norv�gien Nynorsk"
LangString lng_pa $(LANG_FRENCH) "pa  Pendjabi"
LangString lng_pl $(LANG_FRENCH) "po  Polonais"
LangString lng_pt $(LANG_FRENCH) "pt  Portugais"
LangString lng_pt_BR $(LANG_FRENCH) "pt_BR Portugais Br�sil"
LangString lng_ru $(LANG_FRENCH) "ru  Russe"
LangString lng_rw $(LANG_FRENCH) "rw  Kinyarwanda"
LangString lng_sk $(LANG_FRENCH) "sk  Slovaque"
LangString lng_sl $(LANG_FRENCH) "sl  Slov�ne"
LangString lng_sq $(LANG_FRENCH) "sq  Albanian"
LangString lng_sr $(LANG_FRENCH) "sr  Serbe"
LangString lng_sr@Latn $(LANG_FRENCH) "sr@Latn  Serbe en notation latine"
LangString lng_sv $(LANG_FRENCH) "sv  Su�dois"
LangString lng_tr $(LANG_FRENCH) "tr  Turc"
LangString lng_uk $(LANG_FRENCH) "uk  Ukrainien"
LangString lng_vi $(LANG_FRENCH) "vi  Vietnamese"
LangString lng_zh_CN $(LANG_FRENCH) "zh_CH  Chinois simplifi�"
LangString lng_zh_TW $(LANG_FRENCH) "zh_TW  Chinois traditionnel"




; uninstallation options
LangString lng_UInstOpt   ${LANG_FRENCH} "Options de d�sinstallation"

; uninstallation options subtitle
LangString lng_UInstOpt1  ${LANG_FRENCH} "Choisissez parmi les options additionnelles"

; Ask to purge the personal preferences
LangString lng_PurgePrefs ${LANG_FRENCH} "Conserver les pr�f�rences personnelles"
