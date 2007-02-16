; #######################################
; catalan.nsh
; catalan language strings for inkscape installer
; windows code page: 1252
; Authors:
; xavi.conde@gmail.com
; inkscape@softcatala.net
;
; 27 july 2006 new languages en_CA, en_GB, fi, hr, mn, ne, rw, sq
; 11 august 2006 new languages dz bg
; 24 october 2006 new languages en_US@piglatin, th
; 3rd December 2006 new languages eu km
; 14th December 2006 new lng_DeletePrefs, lng_DeletePrefsDesc, lng_WANT_UNINSTALL_BEFORE and lng_OK_CANCEL_DESC
; february 15 2007 new language bn, en_AU, eo, id, ro

!insertmacro MUI_LANGUAGE "Catalan"

; Product name
;LangString lng_Caption   ${LANG_CATALAN} "${PRODUCT_NAME} -- Open Source Scalable Vector Graphics Editor"
LangString lng_Caption   ${LANG_CATALAN} "${PRODUCT_NAME} -- Editor de gr�fics vectorials escalables de codi obert"

; Button text "Next >" for the license page
; LangString lng_LICENSE_BUTTON   ${LANG_CATALAN} "Next >"
LangString lng_LICENSE_BUTTON   ${LANG_CATALAN} "Seg�ent >"

; Bottom text for the license page
; LangString lng_LICENSE_BOTTOM_TEXT   ${LANG_CATALAN} "$(^Name) is released under the GNU General Public License (GPL). The license is provided here for information purposes only. $_CLICK"
LangString lng_LICENSE_BOTTOM_TEXT   ${LANG_CATALAN} "L'$(^Name) s'ha alliberat sota la Llic�ncia P�blica General de GNU (GPL). La llic�ncia es proporciona aqu� nom�s per raons informatives. $_CLICK"
;has been installed by different user
;LangString lng_DIFFERENT_USER ${LANG_CATALAN} "Inkscape has been installed by user $0.$\r$\nIf you continue you might not complete successfully!$\r$\nPlease log in as $0 and try again."
LangString lng_DIFFERENT_USER ${LANG_CATALAN} "L'usuari $0.$\r ha instal�lat l'Inkscape.$\nSi continueu, �s possible que no acabeu correctament.$\r$\nEntreu com a $0 i proveu de nou."

; want to uninstall before install
LangString lng_WANT_UNINSTALL_BEFORE ${LANG_CATALAN} "$R1 has already been installed. $\nDo you want to remove the previous version before installing $(^Name) ?"

; press OK to continue press Cancel to abort
LangString lng_OK_CANCEL_DESC ${LANG_CATALAN} "$\n$\nPress OK to continue or press CANCEL to abort."

;you have no admin rigths
;LangString lng_NO_ADMIN ${LANG_CATALAN} "You do not have administrator privileges.$\r$\nInstalling Inkscape for all users might not complete successfully.$\r$\nUncheck the 'for all users' option."
LangString lng_NO_ADMIN ${LANG_CATALAN} "No teniu privilegis d'administrador.$\r$\n�s possible que la instal�laci� per a tots els usuaris no es completi correctament.$\r$\nInhabiliteu l'opci� 'per a tots els usuaris'."

;win9x is not supported
;LangString lng_NOT_SUPPORTED ${LANG_CATALAN} "Inkscape is known not to run under Windows 95/98/ME!$\r$\nPlease check the official website for detailed information."
LangString lng_NOT_SUPPORTED ${LANG_CATALAN} "L'Inkscape no funciona correctament sobre Windows 95/98/ME.$\r$\nConsulteu el lloc web per a obtenir informaci� detallada."

; Full install type
; LangString lng_Full $(LANG_CATALAN) "Full"
LangString lng_Full $(LANG_CATALAN) "Completa"

; Optimal install type
; LangString lng_Optimal $(LANG_CATALAN) "Optimal"
LangString lng_Optimal $(LANG_CATALAN) "�ptima"

; Minimal install type
; LangString lng_Minimal $(LANG_CATALAN) "Minimal"
LangString lng_Minimal $(LANG_CATALAN) "M�nima"

; Core install section
; LangString lng_Core $(LANG_CATALAN) "${PRODUCT_NAME} SVG Editor (required)"
LangString lng_Core $(LANG_CATALAN) "L'editor SVG ${PRODUCT_NAME} (requerit)"

; Core install section description
; LangString lng_CoreDesc $(LANG_CATALAN) "Core ${PRODUCT_NAME} files and dlls"
LangString lng_CoreDesc $(LANG_CATALAN) "Fitxers i dlls per a l'${PRODUCT_NAME}"

; GTK+ install section
;LangString lng_GTKFiles $(LANG_CATALAN) "GTK+ Runtime Environment (required)"
LangString lng_GTKFiles $(LANG_CATALAN) "Entorn d'execuci� GTK+ (requerit)"

; GTK+ install section description
; LangString lng_GTKFilesDesc $(LANG_CATALAN) "A multi-platform GUI toolkit, used by ${PRODUCT_NAME}"
LangString lng_GTKFilesDesc $(LANG_CATALAN) "Un joc d'eines d'interf�cies gr�fiques multi-plataforma, usat per l'${PRODUCT_NAME}"

; shortcuts install section
; LangString lng_Shortcuts $(LANG_CATALAN) "Shortcuts"
LangString lng_Shortcuts $(LANG_CATALAN) "Dreceres"

; shortcuts install section description
; LangString lng_ShortcutsDesc $(LANG_CATALAN) "Shortcuts for starting ${PRODUCT_NAME}"
LangString lng_ShortcutsDesc $(LANG_CATALAN) "Dreceres per a iniciar l'${PRODUCT_NAME}"

; All user install section
; LangString lng_Alluser $(LANG_CATALAN) "for all users"
LangString lng_Alluser $(LANG_CATALAN) "Per a tots els usuaris"

; All user install section description
; LangString lng_AlluserDesc $(LANG_CATALAN) "Install this application for anyone who uses this computer (all users)"
LangString lng_AlluserDesc $(LANG_CATALAN) "Instal�la aquesta aplicaci� per a tots els usuaris que fan servir aquest ordinador"

; Desktop section
; LangString lng_Desktop $(LANG_CATALAN) "Desktop"
LangString lng_Desktop $(LANG_CATALAN) "Escriptori"

; Desktop section description
; LangString lng_DesktopDesc $(LANG_CATALAN) "Create a shortcut to ${PRODUCT_NAME} on the Desktop"
LangString lng_DesktopDesc $(LANG_CATALAN) "Crea una drecera cap a l'${PRODUCT_NAME} a l'Escriptori"

; Start Menu  section
; LangString lng_Startmenu $(LANG_CATALAN) "Start Menu"
LangString lng_Startmenu $(LANG_CATALAN) "Men� Inici"

; Start Menu section description
; LangString lng_StartmenuDesc $(LANG_CATALAN) "Create a Start Menu entry for ${PRODUCT_NAME}"
LangString lng_StartmenuDesc $(LANG_CATALAN) "Crea una entrada de men� per a l'${PRODUCT_NAME}"

; Quick launch section
; LangString lng_Quicklaunch $(LANG_CATALAN) "Quick Launch"
LangString lng_Quicklaunch $(LANG_CATALAN) "Inici r�pid"

; Quick launch section description
; LangString lng_QuicklaunchDesc $(LANG_CATALAN) "Create a shortcut to ${PRODUCT_NAME} on the Quick Launch toolbar"
LangString lng_QuicklaunchDesc $(LANG_CATALAN) "Crea una drecera cap a l'${PRODUCT_NAME} a la barra d'eines d'Inici r�pid"

; File type association for editing
; LangString lng_SVGWriter ${LANG_CATALAN} "Open SVG files with ${PRODUCT_NAME}"
LangString lng_SVGWriter ${LANG_CATALAN} "Obre els fitxers SVG amb l'${PRODUCT_NAME}"

; File type association for editing description
; LangString lng_SVGWriterDesc ${LANG_CATALAN} "Select ${PRODUCT_NAME} as default editor for SVG files"
LangString lng_SVGWriterDesc ${LANG_CATALAN} "Seleciona l'${PRODUCT_NAME} com a editor predeterminat per als fitxers SVG"

; Context Menu
; LangString lng_ContextMenu ${LANG_CATALAN} "Context Menu"
LangString lng_ContextMenu ${LANG_CATALAN} "Men� contextual"

; Context Menu description
; LangString lng_ContextMenuDesc ${LANG_CATALAN} "Add ${PRODUCT_NAME} into the Context Menu for SVG files"
LangString lng_ContextMenuDesc ${LANG_CATALAN} "Afegeix l'${PRODUCT_NAME} al men� contextual per als fitxers SVG"

; remove personal preferences
LangString lng_DeletePrefs ${LANG_CATALAN} "Delete personal preferences"

; remove personal preferences description
LangString lng_DeletePrefsDesc ${LANG_CATALAN} "Delete personal preferences leftover from previous installations"


; Additional files section
; LangString lng_Addfiles $(LANG_CATALAN) "Additional Files"
LangString lng_Addfiles $(LANG_CATALAN) "Fitxers addicionals"

; Additional files section description
; LangString lng_AddfilesDesc $(LANG_CATALAN) "Additional Files"
LangString lng_AddfilesDesc $(LANG_CATALAN) "Fitxers addicionals"

; Examples section
; LangString lng_Examples $(LANG_CATALAN) "Examples"
LangString lng_Examples $(LANG_CATALAN) "Exemples"

; Examples section description
; LangString lng_ExamplesDesc $(LANG_CATALAN) "Examples using ${PRODUCT_NAME}"
LangString lng_ExamplesDesc $(LANG_CATALAN) "Exemples d'�s de l'${PRODUCT_NAME}"

; Tutorials section
; LangString lng_Tutorials $(LANG_CATALAN) "Tutorials"
LangString lng_Tutorials $(LANG_CATALAN) "Tutorials"

; Tutorials section description
; LangString lng_TutorialsDesc $(LANG_CATALAN) "Tutorials using ${PRODUCT_NAME}"
LangString lng_TutorialsDesc $(LANG_CATALAN) "Tutorials d'�s de l'${PRODUCT_NAME}"

; Languages section
; LangString lng_Languages $(LANG_CATALAN) "Translations"
LangString lng_Languages $(LANG_CATALAN) "Traduccions"

; Languages section dscription
; LangString lng_LanguagesDesc $(LANG_CATALAN) "Install various translations for ${PRODUCT_NAME}"
LangString lng_LanguagesDesc $(LANG_CATALAN) "Instal�la m�s traduccions per a l'${PRODUCT_NAME}"

LangString lng_am $(LANG_CATALAN) "am  Amharic"
LangString lng_az $(LANG_CATALAN) "az  Azerbaijani"
; LangString lng_be $(LANG_CATALAN) "be  Byelorussian"
LangString lng_be $(LANG_CATALAN) "be  Biel�lor�s"
LangString lng_bg $(LANG_CATALAN) "bg  Bulgarian"
LangString lng_bn $(LANG_CATALAN) "bn  Bengali"
; LangString lng_ca $(LANG_CATALAN) "ca  Catalan"
LangString lng_ca $(LANG_CATALAN) "ca  Catal�"
; LangString lng_cs $(LANG_CATALAN) "cs  Czech"
  LangString lng_cs $(LANG_CATALAN) "cs  Txec"
; LangString lng_da $(LANG_CATALAN) "da  Danish"
  LangString lng_da $(LANG_CATALAN) "da  Dan�s"
; LangString lng_de $(LANG_CATALAN) "de  German"
  LangString lng_de $(LANG_CATALAN) "de  Alemany"
LangString lng_dz $(LANG_CATALAN) "dz  Dzongkha"
; LangString lng_el $(LANG_CATALAN) "el  Greek"
  LangString lng_el $(LANG_CATALAN) "el  Grec"
; LangString lng_en $(LANG_CATALAN) "en  English"
  LangString lng_en $(LANG_CATALAN) "en  Angl�s"
LangString lng_en_AU $(LANG_CATALAN) "en_AU Australian English"
LangString lng_en_CA $(LANG_CATALAN) "en_CA Canadian English"
LangString lng_en_GB $(LANG_CATALAN) "en_GB British English"
LangString lng_en_US@piglatin $(LANG_CATALAN) "en_US@piglatin Pig Latin"
LangString lng_eo $(LANG_CATALAN) "eo  Esperanto"
; LangString lng_es $(LANG_CATALAN) "es  Spanish"
  LangString lng_es $(LANG_CATALAN) "es  Espanyol"
; LangString lng_es_MX $(LANG_CATALAN) "es_MX  Mexican Spanish"
  LangString lng_es_MX $(LANG_CATALAN) "es_MX  Espanyol mexic�"
; LangString lng_et $(LANG_CATALAN) "es  Estonian"
  LangString lng_et $(LANG_CATALAN) "et  Estoni�"
  LangString lng_eu $(LANG_CATALAN) "eu  Basque"
  LangString lng_fi $(LANG_CATALAN) "fi  Finish"
; LangString lng_fr $(LANG_CATALAN) "fr  French"
  LangString lng_fr $(LANG_CATALAN) "fr  Franc�s"
; LangString lng_ga $(LANG_CATALAN) "ga  Irish"
  LangString lng_ga $(LANG_CATALAN) "ga  Irland�s"
; LangString lng_gl $(LANG_CATALAN) "gl  Gallegan"
  LangString lng_gl $(LANG_CATALAN) "gl  Gallec"
  LangString lng_hr $(LANG_CATALAN) "hr  Croatian"
; LangString lng_hu $(LANG_CATALAN) "hu  Hungarian"
  LangString lng_hu $(LANG_CATALAN) "hu  Hongar�s"
LangString lng_id $(LANG_CATALAN) "id  Indonesian"
; LangString lng_it $(LANG_CATALAN) "it  Italian"
  LangString lng_it $(LANG_CATALAN) "it  Itali�"
; LangString lng_ja $(LANG_CATALAN) "ja  Japanese"
  LangString lng_ja $(LANG_CATALAN) "ja  Japon�s"
  LangString lng_km $(LANG_CATALAN) "km  Khmer"
; LangString lng_ko $(LANG_CATALAN) "ko  Korean"
  LangString lng_ko $(LANG_CATALAN) "ko  Core�"
; LangString lng_lt $(LANG_CATALAN) "lt  Lithuanian"
  LangString lng_lt $(LANG_CATALAN) "lt  Litu�"
; LangString lng_mk $(LANG_CATALAN) "mk  Macedonian"
  LangString lng_mk $(LANG_CATALAN) "mk  Macedoni"
LangString lng_mn $(LANG_CATALAN) "mn  Mongolian"
LangString lng_ne $(LANG_CATALAN) "ne  Nepali"
; LangString lng_nb $(LANG_CATALAN) "nb  Norwegian Bokm�l"
  LangString lng_nb $(LANG_CATALAN) "nb  Noruec Bokm�l"
; LangString lng_nl $(LANG_CATALAN) "nl  Dutch"
  LangString lng_nl $(LANG_CATALAN) "nl  Holand�s"
; LangString lng_nn $(LANG_CATALAN) "nn  Norwegian Nynorsk"
  LangString lng_nn $(LANG_CATALAN) "nn  Noruec Nynorsk"
  LangString lng_pa $(LANG_CATALAN) "pa  Panjabi"
; LangString lng_pl $(LANG_CATALAN) "po  Polish"
  LangString lng_pl $(LANG_CATALAN) "po  Polon�s"
; LangString lng_pt $(LANG_CATALAN) "pt  Portuguese"
  LangString lng_pt $(LANG_CATALAN) "pt  Portugu�s"
; LangString lng_pt_BR $(LANG_CATALAN) "pt_BR Brazilian Portuguese"
  LangString lng_pt_BR $(LANG_CATALAN) "pt_BR Portugu�s brasiler"
LangString lng_ro $(LANG_CATALAN) "ro  Romanian"
; LangString lng_ru $(LANG_CATALAN) "ru  Russian"
LangString lng_ru $(LANG_CATALAN) "ru  Rus"
LangString lng_rw $(LANG_CATALAN) "rw  Kinyarwanda"
; LangString lng_sk $(LANG_CATALAN) "sk  Slovak"
LangString lng_sk $(LANG_CATALAN) "sk  Eslovac"
; LangString lng_sl $(LANG_CATALAN) "sl  Slovenian"
LangString lng_sl $(LANG_CATALAN) "sl  Esloveni"
LangString lng_sq $(LANG_CATALAN) "sq  Albanian"
; LangString lng_sr $(LANG_CATALAN) "sr  Serbian"
LangString lng_sr $(LANG_CATALAN) "sr  Serbi"
; LangString lng_sr@Latn $(LANG_CATALAN) "sr@Latn  Serbian in Latin script"
LangString lng_sr@Latn $(LANG_CATALAN) "sr@Latn  Serbi en alfabet llat�"
; LangString lng_sv $(LANG_CATALAN) "sv  Swedish"
LangString lng_sv $(LANG_CATALAN) "sv  Suec"
LangString lng_th $(LANG_CATALAN) "th  Thai"
; LangString lng_tr $(LANG_CATALAN) "tr  Turkish"
LangString lng_tr $(LANG_CATALAN) "tr  Turc"
; LangString lng_uk $(LANG_CATALAN) "uk  Ukrainian"
LangString lng_uk $(LANG_CATALAN) "uk  Ucra�n�s"
;LangString lng_vi $(LANG_ENGLISH) "vi  Vietnamese"
LangString lng_vi $(LANG_CATALAN) "vi  Vietnamese"
; LangString lng_zh_CN $(LANG_CATALAN) "zh_CH  Simplifed Chinese"
LangString lng_zh_CN $(LANG_CATALAN) "zh_CH  Xin�s simplificat"
;LangString lng_zh_TW $(LANG_CATALAN) "zh_TW  Traditional Chinese"
LangString lng_zh_TW $(LANG_CATALAN) "zh_TW  Xin�s tradicional"




; uninstallation options
; LangString lng_UInstOpt   ${LANG_CATALAN} "Uninstallation Options"
LangString lng_UInstOpt   ${LANG_CATALAN} "Opcions de desinstal�laci�"

; uninstallation options subtitle
; LangString lng_UInstOpt1  ${LANG_CATALAN} "Please make your choices for additional options"
LangString lng_UInstOpt1  ${LANG_CATALAN} "Seleccioneu les vostres opcions addicionals"

; Ask to purge the personal preferences
; LangString lng_PurgePrefs ${LANG_CATALAN} "Keep personal Preferences"
LangString lng_PurgePrefs ${LANG_CATALAN} "Conserva les prefer�ncies personals"
