; #######################################
; spanish.nsh
; spanish language strings for inkscape installer
; windows code page: 1252
; Authors:
; Adib Taraben theAdib@yahoo.com
; Lucas Vieites aka basurero
; 27 july 2006 new languages en_CA, en_GB, fi, hr, mn, ne, rw, sq
; 11 august 2006 new languages dz bg
; 24 october 2006 new languages en_US@piglatin, th
; 3rd December 2006 new languages eu km
; 14th December 2006 new lng_DeletePrefs, lng_DeletePrefsDesc, lng_WANT_UNINSTALL_BEFORE and lng_OK_CANCEL_DESC
; february 15 2007 new language bn, en_AU, eo, id, ro
; april 11 2007 new language he
; october 2007 new language ca@valencian

!insertmacro MUI_LANGUAGE "Spanish"

; Product name
LangString lng_Caption   ${LANG_SPANISH} "${PRODUCT_NAME} -- Editor de gr�ficos vectoriales escalables (SVG) de c�digo abierto"

; Button text "Next >" for the license page
LangString lng_LICENSE_BUTTON   ${LANG_SPANISH} "Siguiente >"

; Bottom text for the license page
LangString lng_LICENSE_BOTTOM_TEXT   ${LANG_SPANISH} "$(^Name) se publica bajo la Licencia P�blica General GNU (GPL). Esta licencia se muestra aqu� solamente como informaci�n. $_CLICK"

;has been installed by different user
LangString lng_DIFFERENT_USER ${LANG_SPANISH} "Inkscape has been installed by user $0.$\r$\nIf you continue you might not complete successfully!$\r$\nPlease log in as $0 and try again."

; want to uninstall before install
LangString lng_WANT_UNINSTALL_BEFORE ${LANG_SPANISH} "$R1 has already been installed. $\nDo you want to remove the previous version before installing $(^Name) ?"

; press OK to continue press Cancel to abort
LangString lng_OK_CANCEL_DESC ${LANG_SPANISH} "$\n$\nPress OK to continue or press CANCEL to abort."

;you have no admin rigths
LangString lng_NO_ADMIN ${LANG_SPANISH} "You do not have administrator privileges.$\r$\nInstalling Inkscape for all users might not complete successfully.$\r$\nUncheck the 'for all users' option."

;win9x is not supported
LangString lng_NOT_SUPPORTED ${LANG_SPANISH} "Inkscape is known not to run under Windows 95/98/ME!$\r$\nPlease check the official website for detailed information."

; Full install type
LangString lng_Full $(LANG_SPANISH) "Completa"

; Optimal install type
LangString lng_Optimal $(LANG_SPANISH) "�ptima"

; Minimal install type
LangString lng_Minimal $(LANG_SPANISH) "M�nima"

; Core install section
LangString lng_Core $(LANG_SPANISH) "${PRODUCT_NAME} Editor SVG (requerido)"

; Core install section description
LangString lng_CoreDesc $(LANG_SPANISH) "Archivos ${PRODUCT_NAME} b�sicos y dlls"

; GTK+ install section
LangString lng_GTKFiles $(LANG_SPANISH) "GTK+ Runtime Environment (requerido)"

; GTK+ install section description
LangString lng_GTKFilesDesc $(LANG_SPANISH) "Un conjunto de herramientas GUI, utilizado por ${PRODUCT_NAME}"

; shortcuts install section
LangString lng_Shortcuts $(LANG_SPANISH) "Acceso directo"

; shortcuts install section description
LangString lng_ShortcutsDesc $(LANG_SPANISH) "Accesos directos para iniciar ${PRODUCT_NAME}"

; All user install section
LangString lng_Alluser $(LANG_SPANISH) "para todos los usuarios"

; All user install section description
LangString lng_AlluserDesc $(LANG_SPANISH) "Instalar esta aplicaci�n para todos los que utilizan este equipo (todos los usuarios)"

; Desktop section
LangString lng_Desktop $(LANG_SPANISH) "Escritorio"

; Desktop section description
LangString lng_DesktopDesc $(LANG_SPANISH) "Crear un acceso directo a ${PRODUCT_NAME} en el escritorio"

; Start Menu  section
LangString lng_Startmenu $(LANG_SPANISH) "Men� Inicio"

; Start Menu section description
LangString lng_StartmenuDesc $(LANG_SPANISH) "Crear una entrada en el men� Inicio para ${PRODUCT_NAME}"

; Quick launch section
LangString lng_Quicklaunch $(LANG_SPANISH) "Acceso r�pido"

; Quick launch section description
LangString lng_QuicklaunchDesc $(LANG_SPANISH) "Crear un acceso directo a ${PRODUCT_NAME} en la barra de acceso r�pido"

; File type association for editing
LangString lng_SVGWriter ${LANG_SPANISH} "Abrir archivos SVG con ${PRODUCT_NAME}"

; File type association for editing description
LangString lng_SVGWriterDesc ${LANG_SPANISH} "Seleccionar ${PRODUCT_NAME} como editor predeterminado para archivos"

; Context Menu
LangString lng_ContextMenu ${LANG_SPANISH} "Men� contextual"

; Context Menu description
LangString lng_ContextMenuDesc ${LANG_SPANISH} "A�adir ${PRODUCT_NAME} al men� contextual para archivos SVG"

; remove personal preferences
LangString lng_DeletePrefs ${LANG_SPANISH} "Delete personal preferences"

; remove personal preferences description
LangString lng_DeletePrefsDesc ${LANG_SPANISH} "Delete personal preferences leftover from previous installations"

; Additional files section
LangString lng_Addfiles $(LANG_SPANISH) "Archivos adicionales"

; Additional files section description
LangString lng_AddfilesDesc $(LANG_SPANISH) "Archivos adicionales"

; Examples section
LangString lng_Examples $(LANG_SPANISH) "Ejemplos"

; Examples section description
LangString lng_ExamplesDesc $(LANG_SPANISH) "Ejemplos de uso de ${PRODUCT_NAME}"

; Tutorials section
LangString lng_Tutorials $(LANG_SPANISH) "Tutoriales"

; Tutorials section description
LangString lng_TutorialsDesc $(LANG_SPANISH) "Tutoriales del uso de ${PRODUCT_NAME}"


; Languages section
LangString lng_Languages $(LANG_SPANISH) "Traducciones"

; Languages section dscription
LangString lng_LanguagesDesc $(LANG_SPANISH) "Instalar varias traducciones para ${PRODUCT_NAME}"

LangString lng_am $(LANG_SPANISH) "am  Amharic"
LangString lng_az $(LANG_SPANISH) "az  Azerbaijani"
LangString lng_be $(LANG_SPANISH) "be  Byelorussian"
LangString lng_bg $(LANG_SPANISH) "bg  Bulgarian"
LangString lng_bn $(LANG_SPANISH) "bn  Bengali"
LangString lng_ca $(LANG_SPANISH) "ca  Catalan"
LangString lng_ca@valencia $(LANG_SPANISH) "ca@valencia  Valencian Catalan"
LangString lng_cs $(LANG_SPANISH) "cs  Czech"
LangString lng_da $(LANG_SPANISH) "da  Danish"
LangString lng_de $(LANG_SPANISH) "de  German"
LangString lng_dz $(LANG_SPANISH) "dz  Dzongkha"
LangString lng_el $(LANG_SPANISH) "el  Greek"
LangString lng_en $(LANG_SPANISH) "en  English"
LangString lng_en_AU $(LANG_SPANISH) "en_AU Australian English"
LangString lng_en_CA $(LANG_SPANISH) "en_CA Canadian English"
LangString lng_en_GB $(LANG_SPANISH) "en_GB British English"
LangString lng_en_US@piglatin $(LANG_SPANISH) "en_US@piglatin Pig Latin"
LangString lng_eo $(LANG_SPANISH) "eo  Esperanto"
LangString lng_es $(LANG_SPANISH) "es  Espa�ol"
LangString lng_es_MX $(LANG_SPANISH) "es_MX  Espa�ol Mexicano"
LangString lng_et $(LANG_SPANISH) "et  Estonian"
LangString lng_eu $(LANG_SPANISH) "eu  Basque"
LangString lng_fi $(LANG_SPANISH) "fi  Finnish"
LangString lng_fr $(LANG_SPANISH) "fr  French"
LangString lng_ga $(LANG_SPANISH) "ga  Irish"
LangString lng_gl $(LANG_SPANISH) "gl  Gallegan"
LangString lng_he $(LANG_SPANISH) "he  Hebrew"
LangString lng_hr $(LANG_SPANISH) "hr  Croatian"
LangString lng_hu $(LANG_SPANISH) "hu  Hungarian"
LangString lng_id $(LANG_SPANISH) "id  Indonesian"
LangString lng_it $(LANG_SPANISH) "it  Italian"
LangString lng_ja $(LANG_SPANISH) "ja  Japanese"
LangString lng_km $(LANG_SPANISH) "km  Khmer"
LangString lng_ko $(LANG_SPANISH) "ko  Korean"
LangString lng_lt $(LANG_SPANISH) "lt  Lithuanian"
LangString lng_mk $(LANG_SPANISH) "mk  Macedonian"
LangString lng_mn $(LANG_SPANISH) "mn  Mongolian"
LangString lng_ne $(LANG_SPANISH) "ne  Nepali"
LangString lng_nb $(LANG_SPANISH) "nb  Norwegian Bokm�l"
LangString lng_nl $(LANG_SPANISH) "nl  Dutch"
LangString lng_nn $(LANG_SPANISH) "nn  Norwegian Nynorsk"
LangString lng_pa $(LANG_SPANISH) "pa  Panjabi"
LangString lng_pl $(LANG_SPANISH) "po  Polish"
LangString lng_pt $(LANG_SPANISH) "pt  Portuguese"
LangString lng_pt_BR $(LANG_SPANISH) "pt_BR Brazilian Portuguese"
LangString lng_ro $(LANG_SPANISH) "ro  Romanian"
LangString lng_ru $(LANG_SPANISH) "ru  Russian"
LangString lng_rw $(LANG_SPANISH) "rw  Kinyarwanda"
LangString lng_sk $(LANG_SPANISH) "sk  Slovak"
LangString lng_sl $(LANG_SPANISH) "sl  Slovenian"
LangString lng_sq $(LANG_SPANISH) "sq  Albanian"
LangString lng_sr $(LANG_SPANISH) "sr  Serbian"
LangString lng_sr@Latn $(LANG_SPANISH) "sr@Latn  Serbian in Latin script"
LangString lng_sv $(LANG_SPANISH) "sv  Swedish"
LangString lng_th $(LANG_SPANISH) "th  Thai"
LangString lng_tr $(LANG_SPANISH) "tr  Turkish"
LangString lng_uk $(LANG_SPANISH) "uk  Ukrainian"
LangString lng_vi $(LANG_SPANISH) "vi  Vietnamese"
LangString lng_zh_CN $(LANG_SPANISH) "zh_CH  Simplifed Chinese"
LangString lng_zh_TW $(LANG_SPANISH) "zh_TW  Chinese (Taiwan)"




; uninstallation options
LangString lng_UInstOpt   ${LANG_SPANISH} "Opciones de desinstalaci�n"

; uninstallation options subtitle
LangString lng_UInstOpt1  ${LANG_SPANISH} "Elija sus opciones adicionales"

; Ask to purge the personal preferences
LangString lng_PurgePrefs ${LANG_SPANISH} "Guardar las preferencias personales"
