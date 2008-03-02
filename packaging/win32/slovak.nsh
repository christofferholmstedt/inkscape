; #######################################
; slovak.nsh
; slovak language strings for inkscape installer
; windows code page: 1250
; Authors:
; helix84 helix84@gmail.com (translation for Inkscape 0.44, 0.46)
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
; February 2008 new languages ar, br

!insertmacro MUI_LANGUAGE "Slovak"

; Product name
LangString lng_Caption   ${LANG_SLOVAK} "${PRODUCT_NAME} -- Open source editor SVG grafiky"

; Button text "Next >" for the license page
LangString lng_LICENSE_BUTTON   ${LANG_SLOVAK} "�alej >"

; Bottom text for the license page
LangString lng_LICENSE_BOTTOM_TEXT   ${LANG_SLOVAK} "$(^Name) je mo�n� ��ri� za podmienok General Public License (GPL). Licen�n� zmluva je tu len pre informa�n� ��ely. $_CLICK"

;has been installed by different user
LangString lng_DIFFERENT_USER ${LANG_SLOVAK} "Inkscape nain�taloval pou��vate� $0.$\r$\nIn�tal�cia nemus� spr�vne skon�i�, ak v nej budete pokra�ova�!$\r$\nPros�m, prihl�ste sa ako $0 a spustite in�tal�ciu znova."

; want to uninstall before install
LangString lng_WANT_UNINSTALL_BEFORE ${LANG_SLOVAK} "$R1 u� je nain�talovan�. $\nChcete odstr�ni� predch�dzaj�cu verziu predt�m, ne� nain�talujete $(^Name) ?"

; press OK to continue press Cancel to abort
LangString lng_OK_CANCEL_DESC ${LANG_SLOVAK} "$\n$\nPokra�ujte stla�en�m OK alebo zru�te in�tal�ciu stla�en�m Zru�i�."

;you have no admin rigths
LangString lng_NO_ADMIN ${LANG_SLOVAK} "Nem�te pr�va spr�vcu.$\r$\nIn�tal�cia Inkscape pre v�etk�ch pou��vate�ov nemus� skon�i� �spe�ne.$\r$\nZru�te ozna�enie vo�by �Pre v�etk�ch pou��vate�ov�."

;win9x is not supported
LangString lng_NOT_SUPPORTED ${LANG_SLOVAK} "Inkscape nebe�� na Windows 95/98/ME!$\r$\nPodrobnej�ie inform�cie n�jdete na ofici�lnom webe."

; Full install type
LangString lng_Full $(LANG_SLOVAK) "Pln�"

; Optimal install type
LangString lng_Optimal $(LANG_SLOVAK) "Optim�lna"

; Minimal install type
LangString lng_Minimal $(LANG_SLOVAK) "Minim�lna"

; Core install section
LangString lng_Core $(LANG_SLOVAK) "${PRODUCT_NAME} SVG editor (povinn�)"

; Core install section description
LangString lng_CoreDesc $(LANG_SLOVAK) "S�bory a kni�nice ${PRODUCT_NAME}"

; GTK+ install section
LangString lng_GTKFiles $(LANG_SLOVAK) "GTK+ runtime environment (povinn�)"

; GTK+ install section description
LangString lng_GTKFilesDesc $(LANG_SLOVAK) "Multiplatformov� sada pou��vate�sk�ho rozhrania pou�it�ho v ${PRODUCT_NAME}"

; shortcuts install section
LangString lng_Shortcuts $(LANG_SLOVAK) "Z�stupcovia"

; shortcuts install section description
LangString lng_ShortcutsDesc $(LANG_SLOVAK) "Z�stupcovia pre �tart ${PRODUCT_NAME}"

; All user install section
LangString lng_Alluser $(LANG_SLOVAK) "pre v�etk�ch pou��vate�ov"

; All user install section description
LangString lng_AlluserDesc $(LANG_SLOVAK) "In�talova� aplik�ciu pre kohoko�vek, kto pou��va tento po��ta�. (v�etci pou��vatelia)"

; Desktop section
LangString lng_Desktop $(LANG_SLOVAK) "Plocha"

; Desktop section description
LangString lng_DesktopDesc $(LANG_SLOVAK) "Vytvo�it z�stupcu ${PRODUCT_NAME} na ploche"

; Start Menu  section
LangString lng_Startmenu $(LANG_SLOVAK) "Ponuka �tart"

; Start Menu section description
LangString lng_StartmenuDesc $(LANG_SLOVAK) "Vytvori� pre ${PRODUCT_NAME} polo�ku ve ponuke �tart"

; Quick launch section
LangString lng_Quicklaunch $(LANG_SLOVAK) "Panel r�chleho spustenia"

; Quick launch section description
LangString lng_QuicklaunchDesc $(LANG_SLOVAK) "Vytvori� pre ${PRODUCT_NAME} z�stupcu v paneli r�chleho spustenia"

; File type association for editing
LangString lng_SVGWriter ${LANG_SLOVAK} "Otv�ra� SVG s�bory v ${PRODUCT_NAME}"

; File type association for editing description
LangString lng_SVGWriterDesc ${LANG_SLOVAK} "Vybra� ${PRODUCT_NAME} ako �tandardn� editor pre SVG s�bory"

; Context Menu
LangString lng_ContextMenu ${LANG_SLOVAK} "Kontextov� ponuka"

; Context Menu description
LangString lng_ContextMenuDesc ${LANG_SLOVAK} "Prida� ${PRODUCT_NAME} do kontextov�ho menu pre SVG s�bory"

; remove personal preferences
LangString lng_DeletePrefs ${LANG_SLOVAK} "Zmaza� osobn� nastavenia"

; remove personal preferences description
LangString lng_DeletePrefsDesc ${LANG_SLOVAK} "Zmaza� osobn� nastavenia ponechan� z predch�dzaj�cich in�tal�ci�"


; Additional files section
LangString lng_Addfiles $(LANG_SLOVAK) "�al�ie s�bory"

; Additional files section description
LangString lng_AddfilesDesc $(LANG_SLOVAK) "�al�ie s�bory"

; Examples section
LangString lng_Examples $(LANG_SLOVAK) "Pr�klady"

; Examples section description
LangString lng_ExamplesDesc $(LANG_SLOVAK) "Pr�lady pou��vania ${PRODUCT_NAME}"

; Tutorials section
LangString lng_Tutorials $(LANG_SLOVAK) "Sprievodcovia"

; Tutorials section description
LangString lng_TutorialsDesc $(LANG_SLOVAK) "Sprievodcovia funkciami ${PRODUCT_NAME}"


; Languages section
LangString lng_Languages $(LANG_SLOVAK) "Jazykov� sady"

; Languages section dscription
LangString lng_LanguagesDesc $(LANG_SLOVAK) "Nain�talova� �al�ie jazykov� sady ${PRODUCT_NAME}"

LangString lng_am $(LANG_SLOVAK) "am  amhar�ina"
LangString lng_ar $(LANG_SLOVAK) "ar  Arabic"
LangString lng_az $(LANG_SLOVAK) "az  azerbajd�an�ina"
LangString lng_be $(LANG_SLOVAK) "be  bieloru�tina"
LangString lng_bg $(LANG_SLOVAK) "bg  bulhar�ina"
LangString lng_bn $(LANG_SLOVAK) "bn  beng�l�ina"
LangString lng_br $(LANG_SLOVAK) "br  Breton"
LangString lng_ca $(LANG_SLOVAK) "ca  katal�n�ina"
LangString lng_ca@valencia $(LANG_SLOVAK) "ca@valencia  valencij�ina"
LangString lng_cs $(LANG_SLOVAK) "cs  �e�tina"
LangString lng_da $(LANG_SLOVAK) "da  d�n�ina"
LangString lng_de $(LANG_SLOVAK) "de  nem�ina"
LangString lng_dz $(LANG_SLOVAK) "dz  dzongk�"
LangString lng_el $(LANG_SLOVAK) "el  gr��tina"
LangString lng_en $(LANG_SLOVAK) "en  angli�tina"
LangString lng_en_AU $(LANG_SLOVAK) "en_AU angli�tina (Austr�lia)"
LangString lng_en_CA $(LANG_SLOVAK) "en_CA angli�tina (Kanada)"
LangString lng_en_GB $(LANG_SLOVAK) "en_GB angli�tina (Spojen� kr�ovstvo)"
LangString lng_en_US@piglatin $(LANG_SLOVAK) "en_US@piglatin Pig Latin"
LangString lng_eo $(LANG_SLOVAK) "eo  esperanto"
LangString lng_es $(LANG_SLOVAK) "es  �paniel�ina"
LangString lng_es_MX $(LANG_SLOVAK) "es_MX  �paniel�ina (Mexiko)"
LangString lng_et $(LANG_SLOVAK) "et  est�n�ina"
LangString lng_eu $(LANG_SLOVAK) "eu  baski�tina"
LangString lng_fi $(LANG_SLOVAK) "fi  f�n�ina"
LangString lng_fr $(LANG_SLOVAK) "fr  franc�z�tina"
LangString lng_ga $(LANG_SLOVAK) "ga  �r�ina"
LangString lng_gl $(LANG_SLOVAK) "gl  gal�cij�ina"
LangString lng_he $(LANG_SLOVAK) "he  hebrej�ina"
LangString lng_hr $(LANG_SLOVAK) "hr  chorv�t�ina"
LangString lng_hu $(LANG_SLOVAK) "hu  ma�ar�ina"
LangString lng_id $(LANG_SLOVAK) "id  indon�z�tina"
LangString lng_it $(LANG_SLOVAK) "it  talian�ina"
LangString lng_ja $(LANG_SLOVAK) "ja  japon�ina"
LangString lng_km $(LANG_SLOVAK) "km  khm�r�ina"
LangString lng_ko $(LANG_SLOVAK) "ko  k�rej�ina"
LangString lng_lt $(LANG_SLOVAK) "lt  litov�ina"
LangString lng_mk $(LANG_SLOVAK) "mk  maced�n�ina"
LangString lng_mn $(LANG_SLOVAK) "mn  mongol�ina"
LangString lng_ne $(LANG_SLOVAK) "ne  nep�l�ina"
LangString lng_nb $(LANG_SLOVAK) "nb  n�rsky bokmal"
LangString lng_nl $(LANG_SLOVAK) "nl  holand�ina"
LangString lng_nn $(LANG_SLOVAK) "nn  n�rsky nynorsk"
LangString lng_pa $(LANG_SLOVAK) "pa  pand��b�ina"
LangString lng_pl $(LANG_SLOVAK) "po  po��tina"
LangString lng_pt $(LANG_SLOVAK) "pt  portugal�ina"
LangString lng_pt_BR $(LANG_SLOVAK) "pt_BR portugal�ina (Braz�lia)"
LangString lng_ro $(LANG_SLOVAK) "ro  rumun�ina"
LangString lng_ru $(LANG_SLOVAK) "ru  ru�tina"
LangString lng_rw $(LANG_SLOVAK) "rw  rwand�ina"
LangString lng_sk $(LANG_SLOVAK) "sk  sloven�ina"
LangString lng_sl $(LANG_SLOVAK) "sl  slovin�ina"
LangString lng_sq $(LANG_SLOVAK) "sq  alb�n�ina"
LangString lng_sr $(LANG_SLOVAK) "sr  srb�ina"
LangString lng_sr@latin $(LANG_SLOVAK) "sr@latin  srb�ina (latinka)"
LangString lng_sv $(LANG_SLOVAK) "sv  �v�d�ina"
LangString lng_th $(LANG_SLOVAK) "th  thaj�ina"
LangString lng_tr $(LANG_SLOVAK) "tr  ture�tina"
LangString lng_uk $(LANG_SLOVAK) "uk  ukrajin�ina"
LangString lng_vi $(LANG_SLOVAK) "vi  Vietnamese"
LangString lng_zh_CN $(LANG_SLOVAK) "zh_CH  ��n�tina (zjednodu�en�)"
LangString lng_zh_TW $(LANG_SLOVAK) "zh_TW  ��n�tina (tradi�n�)"




; uninstallation options
LangString lng_UInstOpt   ${LANG_SLOVAK} "Mo�nosti dein�tal�cie"

; uninstallation options subtitle
LangString lng_UInstOpt1  ${LANG_SLOVAK} "Zvo�te pros�m �al�ie mo�nosti"

; Ask to purge the personal preferences
LangString lng_PurgePrefs ${LANG_SLOVAK} "Ponecha� osobn� nastavenia"

LangString lng_RETRY_CANCEL_DESC ${LANG_SLOVAK} "$\n$\nPokra�ujte stal�en�m Znovu alebo ukon�ite stla�en�m Zru�i�."

LangString lng_ClearDirectoryBefore ${LANG_SLOVAK} "${PRODUCT_NAME} mus� by� nain�talovan� do pr�zdneho adres�ra. Adres�r $INSTDIR nie je pr�zdny. Pros�m, najprv tento adres�r vy�istite!$(lng_RETRY_CANCEL_DESC)"

LangString lng_UninstallLogNotFound ${LANG_SLOVAK} "$INSTDIR\uninstall.log nebol n�jden�!$\r$\nPros�m, odin�talujte ru�n�m vy�isten�m adres�ra $INSTDIR !"

LangString lng_FileChanged ${LANG_SLOVAK} "S�bor $filename sa po in�tal�cii zmenil.$\r$\nChcete ho napriek tomu vymaza�?"

LangString lng_Yes ${LANG_SLOVAK} "�no"

LangString lng_AlwaysYes ${LANG_SLOVAK} "�no v�etky"

LangString lng_No ${LANG_SLOVAK} "Nie"

LangString lng_AlwaysNo ${LANG_SLOVAK} "Nie v�etky"
