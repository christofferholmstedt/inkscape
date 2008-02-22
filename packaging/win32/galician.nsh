; #######################################
; galician.nsh
; galician language strings for inkscape installer
; windows code page: 1252
; Authors:
; Leandro Regueiro leandro.regueiro@gmail.com
;
; 27 july 2006 new languages en_CA, en_GB, fi, hr, mn, ne, rw, sq
; 11 august 2006 new languages dz bg
; 24 october 2006 new languages en_US@piglatin, th
; 3rd December 2006 new languages eu km
; 14th December 2006 new lng_DeletePrefs, lng_DeletePrefsDesc, lng_WANT_UNINSTALL_BEFORE and lng_OK_CANCEL_DESC
; february 15 2007 new language bn, en_AU, eo, id, ro
; april 11 2007 new language he
; october 2007 new language ca@valencian
; January 2008 new uninstaller messages

!insertmacro MUI_LANGUAGE "Galician"

; Product name
LangString lng_Caption   ${LANG_GALICIAN} "${PRODUCT_NAME} -- Editor de Gr�ficos Vectoriais Escalables (SVG) de C�digo Aberto"

; Button text "Next >" for the license page
LangString lng_LICENSE_BUTTON   ${LANG_GALICIAN} "Seguinte >"

; Bottom text for the license page
LangString lng_LICENSE_BOTTOM_TEXT   ${LANG_GALICIAN} "$(^Name) publ�case baixo GNU General Public License (GPL). A licenza am�sase aqu� s� para informalo. $_CLICK"

;has been installed by different user
LangString lng_DIFFERENT_USER ${LANG_GALICIAN} "Inkscape foi instalado polo usuario $0.$\r$\nSe continua pode que non consiga rematar a instalaci�n con �xito!$\r$\nEntre no sistema coma $0 e t�nteo de novo."

; want to uninstall before install
LangString lng_WANT_UNINSTALL_BEFORE ${LANG_GALICIAN} "$R1 xa foi instalado. $\nDesexa eliminar a versi�n anterior antes de instalar $(^Name) ?"

; press OK to continue press Cancel to abort
LangString lng_OK_CANCEL_DESC ${LANG_GALICIAN} "$\n$\nPrema Aceptar para continuar ou prema CANCELAR para abortar."

;you have no admin rigths
LangString lng_NO_ADMIN ${LANG_GALICIAN} "Non ten privilexios de administrador.$\r$\nPode que non consiga rematar con �xito a instalaci�n de Inkscape para t�dolos usuarios.$\r$\nDesmarque a opci�n 'para t�dolos usuarios'."

;win9x is not supported
LangString lng_NOT_SUPPORTED ${LANG_GALICIAN} "S�bese que Inkscape non funciona en Windows 95/98/ME!$\r$\nConsulte o sitio web oficial para obter informaci�n m�is detallada."

; Full install type
LangString lng_Full $(LANG_GALICIAN) "Completa"

; Optimal install type
LangString lng_Optimal $(LANG_GALICIAN) "�ptima"

; Minimal install type
LangString lng_Minimal $(LANG_GALICIAN) "M�nima"

; Core install section
LangString lng_Core $(LANG_GALICIAN) "Editor de SVG ${PRODUCT_NAME} (requirido)"

; Core install section description
LangString lng_CoreDesc $(LANG_GALICIAN) "Ficheiros b�sicos de ${PRODUCT_NAME} e dlls"

; GTK+ install section
LangString lng_GTKFiles $(LANG_GALICIAN) "Ambiente de Execuci�n GTK+ (requirido)"

; GTK+ install section description
LangString lng_GTKFilesDesc $(LANG_GALICIAN) "Un toolkit GUI multiplataforma, usado por ${PRODUCT_NAME}"

; shortcuts install section
LangString lng_Shortcuts $(LANG_GALICIAN) "Accesos Directos"

; shortcuts install section description
LangString lng_ShortcutsDesc $(LANG_GALICIAN) "Accesos directos para iniciar ${PRODUCT_NAME}"

; All user install section
LangString lng_Alluser $(LANG_GALICIAN) "para t�dolos usuarios"

; All user install section description
LangString lng_AlluserDesc $(LANG_GALICIAN) "Instalar esta aplicaci�n para t�dolos que usan este ordenador (t�dolos usuarios)"

; Desktop section
LangString lng_Desktop $(LANG_GALICIAN) "Escritorio"

; Desktop section description
LangString lng_DesktopDesc $(LANG_GALICIAN) "Crear un acceso directo para ${PRODUCT_NAME} no Escritorio"

; Start Menu  section
LangString lng_Startmenu $(LANG_GALICIAN) "Men� Inicio"

; Start Menu section description
LangString lng_StartmenuDesc $(LANG_GALICIAN) "Crear unha entrada no Men� Inicio para ${PRODUCT_NAME}"

; Quick launch section
LangString lng_Quicklaunch $(LANG_GALICIAN) "Inicio R�pido"

; Quick launch section description
LangString lng_QuicklaunchDesc $(LANG_GALICIAN) "Crear un acceso directo para ${PRODUCT_NAME} na barra de Inicio R�pido"

; File type association for editing
LangString lng_SVGWriter ${LANG_GALICIAN} "Abrir ficheiros SVG con ${PRODUCT_NAME}"

; File type association for editing description
LangString lng_SVGWriterDesc ${LANG_GALICIAN} "Seleccionar ${PRODUCT_NAME} coma o editor predeterminado para os ficheiros SVG"

; Context Menu
LangString lng_ContextMenu ${LANG_GALICIAN} "Men� Contextual"

; Context Menu description
LangString lng_ContextMenuDesc ${LANG_GALICIAN} "Engadir ${PRODUCT_NAME} � Men� Contextual dos ficheiros SVG"

; remove personal preferences
LangString lng_DeletePrefs ${LANG_GALICIAN} "Eliminar as configuraci�ns persoais"

; remove personal preferences description
LangString lng_DeletePrefsDesc ${LANG_GALICIAN} "Eliminar as configuraci�ns persoais de instalaci�ns anteriores"


; Additional files section
LangString lng_Addfiles $(LANG_GALICIAN) "Ficheiros Adicionais"

; Additional files section description
LangString lng_AddfilesDesc $(LANG_GALICIAN) "Ficheiros Adicionais"

; Examples section
LangString lng_Examples $(LANG_GALICIAN) "Exemplos"

; Examples section description
LangString lng_ExamplesDesc $(LANG_GALICIAN) "Exemplos de uso de ${PRODUCT_NAME}"

; Tutorials section
LangString lng_Tutorials $(LANG_GALICIAN) "Titoriais"

; Tutorials section description
LangString lng_TutorialsDesc $(LANG_GALICIAN) "Titoriais de ${PRODUCT_NAME}"


; Languages section
LangString lng_Languages $(LANG_GALICIAN) "Traduci�ns"

; Languages section dscription
LangString lng_LanguagesDesc $(LANG_GALICIAN) "Instala varias traduci�ns de ${PRODUCT_NAME}"

LangString lng_am $(LANG_GALICIAN) "am  Amh�rico"
LangString lng_az $(LANG_GALICIAN) "az  Azer�"
LangString lng_be $(LANG_GALICIAN) "be  Bielorruso"
LangString lng_bg $(LANG_GALICIAN) "bg  B�lgaro"
LangString lng_bn $(LANG_GALICIAN) "bn  Bengal�"
LangString lng_ca $(LANG_GALICIAN) "ca  Catal�n"
LangString lng_ca@valencia $(LANG_GALICIAN) "ca@valencia  Catal�n Valenciano"
LangString lng_cs $(LANG_GALICIAN) "cs  Checo"
LangString lng_da $(LANG_GALICIAN) "da  Dan�s"
LangString lng_de $(LANG_GALICIAN) "de  Alem�n"
LangString lng_dz $(LANG_GALICIAN) "dz  Dzongkha"
LangString lng_el $(LANG_GALICIAN) "el  Grego"
LangString lng_en $(LANG_GALICIAN) "en  Ingl�s"
LangString lng_en_AU $(LANG_GALICIAN) "en_AU Ingl�s Australiano"
LangString lng_en_CA $(LANG_GALICIAN) "en_CA Ingl�s Canadense"
LangString lng_en_GB $(LANG_GALICIAN) "en_GB Ingl�s Brit�nico"
LangString lng_en_US@piglatin $(LANG_GALICIAN) "en_US@piglatin Pig Latin"
LangString lng_eo $(LANG_GALICIAN) "eo  Esperanto"
LangString lng_es $(LANG_GALICIAN) "es  Espa�ol"
LangString lng_es_MX $(LANG_GALICIAN) "es_MX  Espa�ol Mexicano"
LangString lng_et $(LANG_GALICIAN) "et  Estonio"
LangString lng_eu $(LANG_GALICIAN) "eu  �uscaro "
LangString lng_fi $(LANG_GALICIAN) "fi  Fin�s"
LangString lng_fr $(LANG_GALICIAN) "fr  Franc�s"
LangString lng_ga $(LANG_GALICIAN) "ga  Ga�lico Irland�s"
LangString lng_gl $(LANG_GALICIAN) "gl  Galego"
LangString lng_he $(LANG_GALICIAN) "he  Hebreo"
LangString lng_hr $(LANG_GALICIAN) "hr  Croata"
LangString lng_hu $(LANG_GALICIAN) "hu  H�ngaro"
LangString lng_id $(LANG_GALICIAN) "id  Indonesio"
LangString lng_it $(LANG_GALICIAN) "it  Italiano"
LangString lng_ja $(LANG_GALICIAN) "ja  Xapon�s"
LangString lng_km $(LANG_GALICIAN) "km  Khmer"
LangString lng_ko $(LANG_GALICIAN) "ko  Coreano"
LangString lng_lt $(LANG_GALICIAN) "lt  Lituano"
LangString lng_mk $(LANG_GALICIAN) "mk  Macedonio"
LangString lng_mn $(LANG_GALICIAN) "mn  Mongol"
LangString lng_ne $(LANG_GALICIAN) "ne  Nepal�"
LangString lng_nb $(LANG_GALICIAN) "nb  Noruegu�s Bokm�l"
LangString lng_nl $(LANG_GALICIAN) "nl  Neerland�s"
LangString lng_nn $(LANG_GALICIAN) "nn  Noruegu�s Nynorsk"
LangString lng_pa $(LANG_GALICIAN) "pa  Punjabi"
LangString lng_pl $(LANG_GALICIAN) "po  Polaco"
LangString lng_pt $(LANG_GALICIAN) "pt  Portugu�s"
LangString lng_pt_BR $(LANG_GALICIAN) "pt_BR Portugu�s Brasileiro"
LangString lng_ro $(LANG_GALICIAN) "ro  Roman�s"
LangString lng_ru $(LANG_GALICIAN) "ru  Ruso"
LangString lng_rw $(LANG_GALICIAN) "rw  Kinyarwanda"
LangString lng_sk $(LANG_GALICIAN) "sk  Eslovaco"
LangString lng_sl $(LANG_GALICIAN) "sl  Eslovenio"
LangString lng_sq $(LANG_GALICIAN) "sq  Alban�s"
LangString lng_sr $(LANG_GALICIAN) "sr  Serbio"
LangString lng_sr@latin $(LANG_GALICIAN) "sr@latin  Serbio con alfabeto Latino"
LangString lng_sv $(LANG_GALICIAN) "sv  Sueco"
LangString lng_th $(LANG_GALICIAN) "th  Tailand�s"
LangString lng_tr $(LANG_GALICIAN) "tr  Turco"
LangString lng_uk $(LANG_GALICIAN) "uk  Ucra�no"
LangString lng_vi $(LANG_GALICIAN) "vi  Vietnamita"
LangString lng_zh_CN $(LANG_GALICIAN) "zh_CH  Chin�s Simplificado"
LangString lng_zh_TW $(LANG_GALICIAN) "zh_TW  Chin�s Tradicional"




; uninstallation options
LangString lng_UInstOpt   ${LANG_GALICIAN} "Opci�ns de Desinstalaci�n"

; uninstallation options subtitle
LangString lng_UInstOpt1  ${LANG_GALICIAN} "Escolla as opci�ns adicionais"

; Ask to purge the personal preferences
LangString lng_PurgePrefs ${LANG_GALICIAN} "Conservar a Configuraci�n persoal"

LangString lng_RETRY_CANCEL_DESC ${LANG_GALICIAN} "$\n$\nPrema VOLVER TENTAR para continuar ou prema CANCELAR para abortar."

LangString lng_ClearDirectoryBefore ${LANG_GALICIAN} "${PRODUCT_NAME} debe instalarse nun directorio baleiro. $INSTDIR non est� baleiro. Baleire este directorio primeiro!$(lng_RETRY_CANCEL_DESC)"

LangString lng_UninstallLogNotFound ${LANG_GALICIAN} "Non se atopou $INSTDIR\uninstall.log!$\r$\nDesinstale baleirando vostede mesmo o directorio $INSTDIR!"

LangString lng_FileChanged ${LANG_GALICIAN} "O ficheiro $filename cambiou despois da instalaci�n.$\r$\nAinda desexa eliminar ese ficheiro?"

LangString lng_Yes ${LANG_GALICIAN} "S�"

LangString lng_AlwaysYes ${LANG_GALICIAN} "sempre responder S�"

LangString lng_No ${LANG_GALICIAN} "Non"

LangString lng_AlwaysNo ${LANG_GALICIAN} "sempre responder Non"
