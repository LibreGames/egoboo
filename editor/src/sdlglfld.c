/*******************************************************************************
*  SDLGLFLD.C                                                                  *
*      - Different basic functions for handling of SDLGL_INPUT-fields          *
*                                                                              *
*  SDLGL-TOOLS                                                                 *
*      (c)2002-2010 Paul Mueller <pmtech@swissonline.ch>                       *
*                                                                              *
*  This program is free software; you can redistribute it and/or modify        *
*  it under the terms of the GNU General Public License as published by        *
*  the Free Software Foundation; either version 2 of the License, or           *
*  (at your option) any later version.                                         *
*                                                                              *
*  This program is distributed in the hope that it will be useful,             *
*  but WITHOUT ANY WARRANTY; without even the implied warranty of              *
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
*  GNU Library General Public License for more details.                        *
*                                                                              *
*  You should have received a copy of the GNU General Public License           *
*  along with this program; if not, write to the Free Software                 *
*  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  *
*******************************************************************************/

/*******************************************************************************
* INCLUDES                                 								       *
*******************************************************************************/

#include <string.h>
#include <ctype.h>


#include "sdlglfld.h"

/*******************************************************************************
* CODE                                   								       *
*******************************************************************************/

/*
 * Name:
 *     sdlglfldValToStr
 * Description:
 *     Converts given value pointed with given type to a string 
 * Input:
 *     type:      Type of value given in 'value'
 *     value *:   Pointer on value of 'type'
 *     val_str *: Where to return the edit-value as string
 *     len:       Maximum length for 'val_str'
 */
static void sdlglfldValToStr(char type, void *value, char *val_str, int len)
{

    char *pdata;


    val_str[0] = 0;     /* Init as empty string */

    pdata = (char *)value;

    if (! pdata) {

        return;

    }

    switch(type) {

        case SDLGL_VAL_NONE:
        case SDLGL_VAL_STRING:
            strncpy(val_str, value, len);
            val_str[len] = 0;               /* Clamp, if needed */
            break;

        case SDLGL_VAL_CHAR:
        case SDLGL_VAL_UCHAR:
            sprintf(val_str, "%d", *(int *)pdata);
            break;

        case SDLGL_VAL_SHORT:
        case SDLGL_VAL_USHORT:
            sprintf(val_str, "%d", *(short int *)pdata);
            break;

        case SDLGL_VAL_INT:
            sprintf(val_str, "%d", *(int *)pdata);
            break;

        case SDLGL_VAL_UINT:
            sprintf(val_str, "%u", *(unsigned int *)pdata);
            break;

        case SDLGL_VAL_FLOAT:
            sprintf(val_str, "%f", *(float *)pdata);
            break;
            
        case SDLGL_VAL_ONECHAR:
            *pdata = *val_str;
            break;

    }

}

/*
 * Name:
 *     sdlglfldStrToVal
 * Description:
 *     Converts the given string to a value and returns it in argument 'value'
 * Input:
 *     type:      Type of value given in 'value'
 *     val_str *: Pointer on value in string format
 *     value *:   Pointer on value to return the converted value in
 *     vallen:    length, if string
 */
static void sdlglfldStrToVal(char type, char *val_str, void *value, int vallen)
{
    
    int   ivalue;
    float fvalue;
    char *pstr;


    if (val_str[0] == 0) {

        return;

    }

    if (type == SDLGL_VAL_STRING || type == SDLGL_VAL_ONECHAR) {

        pstr = (char *)value;
       
        while (*pstr == ' ') {

            pstr++;

        }

        if (type == SDLGL_VAL_STRING) {

            strncpy(pstr, val_str, vallen);
            pstr[vallen] = 0;

        }
        else {

            *pstr = *val_str;

        }

    }
    else {

        /* It's a value */
        sscanf(val_str, "%d", &ivalue);
        
        switch(type) {

            case SDLGL_VAL_CHAR:
                *(char *)value = (char)ivalue;
                break;

            case SDLGL_VAL_UCHAR:
                *(unsigned char *)value = (unsigned char)ivalue;
                break;

            case SDLGL_VAL_SHORT:
                *(short int *)value = (short int)ivalue;
                break;

            case SDLGL_VAL_USHORT:
                *(unsigned short int *)value = (unsigned short int)ivalue;
                break;

            case SDLGL_VAL_INT:
                *(int *)value = (int)ivalue;
                break;

            case SDLGL_VAL_UINT:
                *(unsigned int *)value = (unsigned int)ivalue;
                break;

            case SDLGL_VAL_FLOAT:
                sscanf(val_str, "%f", &fvalue);
                *(float *)value = fvalue;
                break;

        }

    } 

}

/*
 * Name:
 *     sdlglfldFindType
 * Description:
 *     Finds the subcode field in given fields and returns it, if found.
 *     Otherwise it retruns a pointer on an empy field.
 *     All the fields are looked up which have the same 'code' as the base field

 * Input:
 *     base *:     Pointer on first element of list to find the field in.
 *     sdlgl_type: To find
 * Output:
 *     Pointer on field found.
 */
static SDLGL_FIELD *sdlglfldFindType(SDLGL_FIELD *base, char sdlgl_type)
{

    static SDLGL_FIELD DummyField;

    int code;


    code = base -> code;        /* Users code for this block of fields */

    while(base -> sdlgl_type != 0) {

        if (base -> code == code && base -> sdlgl_type == sdlgl_type) {

            return base;

        }

        base++;

    }

    return &DummyField;

}

/*
 * Name:
 *     sdlglfldCalcSliderButton
 * Description:
 *     Calculates the button size and button position in given 'base'
 *     rectangle (slider background).
 *     As direction of the slider always the longer side of the rectangle
 *     is taken into account.
 *     The slider button has a minimum size of 3. The slider itself must
 *     have a minimum size of 3.
 * Input:
 *     slider *: Total rectangle, in which the slider can move
 *     button *: Where to put the size of the slider button
 *     maxval:   Maximum value that can be displayed by slider (min = 0)
 *     actval:   Actual index into slider position (0 is top/left)
 */
static void sdlglfldCalcSliderButton(SDLGL_RECT *slider, SDLGL_RECT *button, int maxval, int actval)
{

    int sliderlen;  /* Length used for calculation of slider    */
    int buttonlen;  /* Size of button                           */
    int buttonpos;  /* Start of button measured from base       */


    /* ------- Assume slider as big as the base (and having same pos) ----- */
    button -> x = slider -> x;
    button -> y = slider -> y;
    button -> w = slider -> w;
    button -> h = slider -> h;
    /* Now get the different values */
    if (slider -> w > slider -> h) {    /* Is a horizontal slider */

        sliderlen = slider -> w;

    }
    else {                          /* Is a vertical slider     */

        sliderlen = slider -> h;

    }

    if (maxval < 1) {

        maxval = 1; /* Prevent division by zero: Buttonsize = sliderlen  */

    }

    buttonlen = sliderlen / maxval;
    if (buttonlen < 3) {

        buttonlen = 3;

    }

    buttonpos = 0;          /* Assume at top of slider, prevent division by zero */
    if (actval > 0) {

        buttonpos = sliderlen * actval / maxval;

    }

    if ((buttonpos + buttonlen) > sliderlen) {

        buttonpos = sliderlen - buttonlen;

    }

    if (slider -> w > slider -> h) {    /* Is a horizontal slider */

        button -> x += buttonpos;
        button -> w =  buttonlen;

    }
    else {                          /* Is a vertical slider     */

        button -> y += buttonpos;
        button -> h =  buttonlen;

    }

}

/*
 * Name:
 *     sdlglfldSliBoxAdjButton
 * Description:
 *     Processes the key input for an edit field.
 * Input:
 *     sb *: Pointer on Sliderbox
 */
static void sdlglfldSliBoxAdjButton(SDLGLFLD_SLIDERBOX *sb)
{

    SDLGL_FIELD *field1, *field2;


    field1 = sdlglfldFindType(sb -> fields, SDLGL_TYPE_SLI_BK);
    field2 = sdlglfldFindType(sb -> fields, SDLGL_TYPE_SLI_BUTTON);
    
    sdlglfldCalcSliderButton(&field1 -> rect, &field2 -> rect, sb -> numelement, sb -> actel);

}

/*
 * Name:
 *     sdlglfldEditProcessFuncKey
 * Description:
 *     Processes the key input for an edit field.
 * Input:
 *     event *:     The input to translate
 *     field *:     Pointer on field to handle 
 *     text *:      To edit
 */
static int sdlglfldEditProcessFuncKey(SDLGL_EVENT *event, SDLGL_FIELD *field, char *text)
{

    char *lp;
    int  size;


    switch (event -> sdlcode) {

    	case SDLK_INSERT:
            /* editfld -> editmode ^= 1; */	/* Switch the insert mode flag */
            field -> fstate ^= SDLGL_FSTATE_EDITINS;
            return 1;

        case SDLK_LEFT:
            if (field -> edit_cur > 0) {

            	(field -> edit_cur)--;	/* Move the cursor left	       */

            }
            return 1;

        case SDLK_RIGHT:
            if (field -> edit_cur < (field -> workval - 1)) {

            	/* Don't move behind the end of string */
                if (text[field -> edit_cur] != 0) {

                    field -> edit_cur++;

                }

            }
            return 1;

        case SDLK_HOME:
            if (event -> modflags & KMOD_CTRL) {

            	/* Delete the input field */
                text[0] = 0;

            }

            field -> edit_cur = 0;		/* Move the cursor home */
            return 1;

        case SDLK_END:
            field -> edit_cur = (char)strlen(text);

            if (field -> edit_cur > (field -> workval - 1)) {

               field -> edit_cur = (char)(field -> workval - 1);

            }
            return 1;

        case SDLK_BACKSPACE:
            /* Remove the char if at end of string */
            if (field -> workval > 0) {

                if (text[field -> edit_cur] == 0 || text[field -> edit_cur + 1] == 0) {

                    text[field -> edit_cur] = 0; /* Delete this char */

                }

                if (field -> edit_cur < (char)(field -> workval - 1)) {

                    field -> edit_cur--;

                }

            }

        case SDLK_DELETE:
            lp = text + field -> edit_cur;
            size = strlen(lp + 1) + 1;	/* Including '\0' at end of string */
            memmove(lp, lp + 1, size);
            return 1;

    }

    return 0;

}

/*
 * Name:
 *     sdlglfldEditStdKey
 * Description:
 *     Processes the key input for an edit field, depending on its type
 * Input:
 *     event *:     The input to translate
 *     field *:     Pointer on field to handle 
 * Output:
 *     > 0: Valid char, changed to upper, if needed 
 */
static char sdlglfldEditStdKey(SDLGL_EVENT *event, SDLGL_FIELD *field)
{

    if (event -> text_key > 0) {

        switch(field -> sub_code) {

            case SDLGL_VAL_CHAR:
            case SDLGL_VAL_UCHAR:
            case SDLGL_VAL_SHORT:
            case SDLGL_VAL_USHORT:
            case SDLGL_VAL_INT:
            case SDLGL_VAL_UINT:
                if (isdigit(event -> text_key)) {
                    return event -> text_key;
                }
                break;
            case SDLGL_VAL_FLOAT:
                if (isdigit(event -> text_key) || event -> text_key == '.') {
                    return event -> text_key;
                }
                break;
            case SDLGL_VAL_ONECHAR:
            case SDLGL_VAL_STRING:
                if (field -> sub_code == SDLGL_VAL_ONECHAR) {

                    return (char)toupper(event -> text_key);

                }
                return event -> text_key;

        }
                
    }            

    return 0;
    
}

/*
 * Name:
 *     sdlglfldChangeSliderBox
 * Description:
 *     Change the slider value into given direction by one. The display
 *     of the slider position has to be updated by the caller.
 * Input:
 *     sb *: Pointer on sb, holding the data for
 *     dir:  Direction to move the slider to
 */
static void sdlglfldChangeSliderBox(SDLGLFLD_SLIDERBOX *sb, int dir)
{

    int elbottom;


    if (dir < 0) {

        if (sb -> elchosen > 0) {

            sb -> elchosen--;
            sb -> actel--;

        }
        else if ((sb -> elchosen == 0) && (sb -> eltop > 0)) {

            /* Change top element, if cursor is at the top of slider box    */
            /* if possible                                                  */
            sb -> eltop--;
            sb -> actel--;

        }

    }
    else if (dir > 0) {

        elbottom = sb -> elvisi - 1;
        if (sb -> elchosen < elbottom) {

            /* Cursor not at bottom of slider box */
            if (sb -> elchosen < (sb -> numelement -1)) {

                sb -> elchosen++;
                sb -> actel++;

            }

        }
        else if (sb -> elchosen == elbottom) {

            /* Change top element, if cursor is at the top of slider box    */
            /* if possible                                                  */
            if (elbottom < (sb -> numelement -1)) {

                sb -> eltop++;
                sb -> actel++;

            }

        }

    }

    /* Adjust position of slider button  */
    sdlglfldSliBoxAdjButton(sb);

}

/*
 * Name:
 *     sdlglfldChooseSliderBox
 * Description:
 *     Chooses an element in a slider box depending on the click from event.
 * Input:
 *     eevnt *: Event to handle
 *     sb *:    Pointer on sb, holding the data for
 */
static void sdlglfldChooseSliderBox(SDLGL_EVENT *event, SDLGLFLD_SLIDERBOX *sb)
{

    int elbottom, chosenbottom;


    /* Get the new actual element, depending on the users click */
    sdlglfldValueFromClick(event, sb -> numelement, &sb -> actel);

    /* Now set the display for the value depending on its position */
    chosenbottom = sb -> elvisi - 1;
    elbottom     = sb -> eltop + chosenbottom;

    if (sb -> actel < sb -> eltop) {    /* Off box on top */

        sb -> eltop    = sb -> actel;   /* Display it at top of box */
        sb -> elchosen = 0;

    }
    else if (sb -> actel > elbottom) {

        sb -> eltop    = sb -> actel - chosenbottom;    /* Display at bottom of box */
        sb -> elchosen = chosenbottom;

    }
    else {      /* Change only chosen in box */

        sb -> elchosen = sb -> actel - sb -> eltop;

    }

    /* Does the update on the button */
    sdlglfldSliBoxAdjButton(sb);

    /* TODO: Add support for horizontal slider boxes   */

}



/*
 * Name:
 *     sdlglfldSliBoxChooseElement
 * Description:
 *     Choses an element in given sliberbox depending on type and position
 *     of mouse click.
 * Input:
 *     event *:    Event to handle
 *     sb *:    Slidebox to use with event
 */
static void sdlglfldSliBoxChooseElement(SDLGL_EVENT *event, SDLGLFLD_SLIDERBOX *sb)
{

    SDLGL_FIELD *field;
    int vchoose, hchoose, elchosen;


    field = event -> field;

    vchoose = sb -> elvisi * event -> mou.y / field -> rect.h;
    hchoose = 0;
    if (sb -> helvisi > 0) {      /* is horzotal slider box */

        hchoose = sb -> helvisi * event -> mou.x / field -> rect.w;
        if (hchoose > 0) {

            hchoose = ((hchoose - 1) * sb -> elvisi);

        }

    }

    vchoose += hchoose;
    elchosen = sb -> eltop + vchoose;
    if (elchosen < sb -> numelement) {  /* Only if a valid element */

        sb -> elchosen = vchoose;
        sb -> actel    = elchosen;

    }

    sdlglfldSliBoxAdjButton(sb);

}

/*
 * Name:
 *     sdlglfldCalcSliBoxElement
 * Description:
 *     Calculates the element size and element position in slider box
 *     rectangle (slider elements box).
 * Input:
 *     sb *: Pointer on sliderbox
 */
static void sdlglfldCalcSliBoxElement(SDLGLFLD_SLIDERBOX *sb)
{

    SDLGL_FIELD *field1, *field2;
    SDLGL_RECT  *base, *target;


    field1 = sdlglfldFindType(sb -> fields, SDLGL_TYPE_SLI_BOX);
    field2 = sdlglfldFindType(sb -> fields, SDLGL_TYPE_SLI_ELEMENT);
    base   = &field1 -> rect;
    target = &field2 -> rect;

    /* ------- Assume slider as big as the base (and having same pos) ----- */
    target -> x = base -> x;
    target -> y = base -> y;
    target -> w = base -> w;

    /* ------- FIXME: Support horizontal slider boxes... */
    target -> h = base -> h / sb -> elvisi;
    target -> y += base -> h * sb -> elchosen / sb -> elvisi;

}

/*
 * Name:
 *     sdlglfldSliderButton
 * Description:
 *     Handles the input of a Slider-Button (drag)
 * Input:
 *     event *:  Event to handle
 *     maxval:   Maximum value that can be chosen by click
 *     value *:  To change, if theres any change at all
 *     slider *: Slider button is moving on this rectangle, field
 */
static void sdlglfldSliderButton(SDLGL_EVENT *event, int maxval, int *value, SDLGL_FIELD *slider)
{

    SDLGL_FIELD *button;
    int dragged;


    if (! event -> field) {

        return;     /* play it save */

    }

    button = event -> field;

    if (button -> fstate & (SDLGL_FSTATE_MOUDRAG | SDLGL_FSTATE_MOUSEOVER)) {

        /* Mouse is dragged in this rectangle */
        dragged = 0;

        if (slider -> rect.w > slider -> rect.h) {      /* Drag horizontally */

            if (event -> mou.w > event -> mou.h) {

                button -> rect.x += event -> mou.w;      /* Slider is moved */
                event -> mou.x = button -> rect.x - slider -> rect.x;
                dragged = 1;

            }

        }
        else {  /* Drag vertically */

            if (event -> mou.h > event -> mou.w) {

                button -> rect.y += event -> mou.h;      /* Slider is moved */
                event -> mou.y = button -> rect.y - slider -> rect.y;
                dragged = 1;

            }

        }

        if (dragged) {

            /* Mimic click on slider background */
            event -> field = slider;
            sdlglfldValueFromClick(event, maxval, value);

        }

    }

}

/*
 * Name:
 *     sdlglfldCheckBox
 * Description:
 *     Handles a checkbox.
 * Input:
 *     event *:    Event to handle
 *     bitmask:    Mask of bit to set / clear 
 *     bitvalue *: Pointer char hilding the flags to handle
 * Output:
 *     Actual state of given bit 
 */
static char sdlglfldCheckBox(SDLGL_EVENT *event, char bitmask, char *bitvalue)
{

    if (event -> sdlcode == SDLGL_KEY_MOULEFT) {

        *bitvalue ^= bitmask;
        
    }
    
    return (char)(*bitvalue & bitmask ? 1 : 0); 

}

/*
 * Name:
 *     sdlglfldRadioButton
 * Description:
 *     Handles a radio button
 * Input:
 *     event *:    Event to handle
 *     bitmask:    Mask of bit to set / clear 
 *     bitvalue *: Pointer char hilding the flags to handle
 */
static char sdlglfldRadioButton(SDLGL_EVENT *event, char bitmask, char *bitvalue)
{

    if (event -> sdlcode == SDLGL_KEY_MOULEFT) {

        *bitvalue = bitmask;       

    }
    
    return (char)(*bitvalue & bitmask ? 1 : 0); 

}

/*
 * Name:
 *     sdlglfldEdit
 * Description:
 *     Handles the given 'event' to edit given value.
 *     All info about the edit-field are taken from the 'event-field'
 * Input:
 *     event *: Holding click position and field
 *     field *: To handle 
 *     text *:  Text to edit
 */
static void sdlglfldEdit(SDLGL_EVENT *event, SDLGL_FIELD *field, char *text)
{
    
    char key;

    
    if (! field) {

        return;

    }

    if (field -> edit_cur < 0) {       /* First time called */

        /* Mimic input of special key */
        event -> sdlcode = SDLK_END;
        sdlglfldEditProcessFuncKey(event, field, text);
        return;

    }


    if (event -> sdlcode == SDLK_RETURN) {

        field -> fstate &= ~SDLGL_FSTATE_HASFOCUS;
        field -> edit_cur = -1;
        /* Change focus to next field */
        sdlglInputSetFocus(field, +1);
        return;

    }

    if (! sdlglfldEditProcessFuncKey(event, field, text)) {

        /* Try standard keys  */
        key = sdlglfldEditStdKey(event, field);

        if (key > 0 && field -> edit_cur >= 0) {    /* Valid cursor position */

            /* Do edit of editfield */
            text[field -> edit_cur] = key;

            /* And move the cursor one position right, if possible */
            if (field -> edit_cur < (field -> workval - 1))  {

                (field -> edit_cur)++;

            }

        }

    }

}


/* ========================================================================== */
/* ============================= THE MAIN ROUTINE(S) ======================== */
/* ========================================================================== */

/*
 * Name:
 *     sdlglfldValueFromClick
 * Description:
 *     Returns the value chosen on a rectangle, given that 'maxvalue'
 *     is the value at the maximum size of the rectangle.
 *     As direction of the slider always the longer side of the rectangle
 *     is taken into account.
 * Input:
 *     event *:  Holding click position and field
 *     maxvalue: At edge of input rectangle
 *     value *:  Where to return the value, if changed.
 */
void sdlglfldValueFromClick(SDLGL_EVENT *event, int maxval, int *value)
{

    SDLGL_FIELD *field;
    int actval;


    if (! event -> field) {

        return;     /* Play it safe */

    }

    field = event -> field;

    if (field -> rect.w > field -> rect.h) {

        actval = maxval * event -> mou.x / field -> rect.w;

    }
    else {

        actval = maxval * event -> mou.y / field -> rect.h;

    }

    *value = actval;

}

/*
 * Name:
 *     sdlglfldSlider
 * Description:
 *     Returns the value chosen on a rectangle, given that 'maxvalue'
 *     is the value at the maximum size of the rectangle.
 *     As direction of the slider always the longer side of the rectangle
 *     is taken into account.
 * Input:
 *     event *:  Holding click position and field
 *     maxvalue: At edge of input rectangle
 *     value *:  Where to return the value, if changed.
 *     button *: Pointer on button to change position, if needed
 */
void sdlglfldSlider(SDLGL_EVENT *event, int maxval, int *value, SDLGL_RECT *button)
{

    SDLGL_FIELD *slider;


    if (event -> field) {

        sdlglfldValueFromClick(event, maxval, value);
        if (button) {   /* Adjust it, if available */

            slider = event -> field;

            sdlglfldCalcSliderButton(&slider -> rect, button, maxval, *value);

        }

    }

}

/*
 * Name:
 *     sdlglfldChangeValue
 * Description:
 *     Changes a value in given direction and stepsize (e.g -1/+1)
 * Input:
 *     dir:     Direction and step (e.g +1/-1)
 *     maxval:  Maximum value that can be chosen by click
 *     value *: To change, if theres any change at all
 */
void sdlglfldChangeValue(int dir, int maxval, int *value)
{

    int newvalue;


    newvalue = *value;
    newvalue += dir;

    if (newvalue < 0 || newvalue > maxval) {

        return;     /* No change */

    }

    *value = newvalue;

}

/*
 * Name:
 *     sdlglfldSliderBox
 * Description:
 *     Handles a sliderbox.
 *     Assumes that 'event -> subcode' holds the type of event to handle.
 *     'event -> code' is for caller.
 * Input:
 *     event *:    Event to handle
 *     sb *:    Slidebox to use with event
 */
void sdlglfldSliderBox(SDLGL_EVENT *event, SDLGLFLD_SLIDERBOX *sb)
{

    SDLGL_FIELD *slider;


    if (! event -> field) {

        return;     /* play it save */

    }
    
    switch(event -> sdlgl_type) {

        case SDLGL_TYPE_SLI_BOX:          /* Chooses element in box   */
            sdlglfldSliBoxChooseElement(event, sb);
            break;

        case SDLGL_TYPE_SLI_BK:        /* Background of slider     */
            sdlglfldChooseSliderBox(event, sb);
            break;

        case SDLGL_TYPE_SLI_BUTTON:    /* Button is 'dragged'  */
            slider = sdlglfldFindType(sb -> fields, SDLGL_TYPE_SLI_BK);
            sdlglfldSliderButton(event, sb -> numelement, &sb -> actel, slider);
            sdlglfldChooseSliderBox(event, sb);
            break;

        case SDLGL_TYPE_SLI_AD:
        case SDLGL_TYPE_SLI_AR:   /* Increment value  */
            sdlglfldChangeSliderBox(sb, +1);
            break;

        case SDLGL_TYPE_SLI_AU:      /* Decrement value  */
        case SDLGL_TYPE_SLI_AL:
            sdlglfldChangeSliderBox(sb, -1);
            break;

    }

    /* Recalc the the element display position */
    sdlglfldCalcSliBoxElement(sb);

}




/*
 * Name:
 *     sdlglfldHandle
 * Description:
 *     Handles all fields, which can be described by a SDLGL_FIELD-Descriptor 
 *     Handles a radio button
 * Input:
 *     event *:    To handle, holding field to work with  
 * Output:
 *     != 0, if the 'event'-input was translated based on the type of field 
 */
int  sdlglfldHandle(SDLGL_EVENT *event)
{

    char val_str[150];
    SDLGL_FIELD *field;


	field = event -> field;

	switch(field -> sdlgl_type) {

		case SDLGL_TYPE_EDIT:
            sdlglfldValToStr(field -> sub_code, field -> pdata, val_str, 127);
		    sdlglfldEdit(event, field, val_str);
            sdlglfldStrToVal(field -> sub_code, val_str, field -> pdata, field -> workval);
		    break;
     
		case SDLGL_TYPE_CHECKBOX:
		    field -> workval = sdlglfldCheckBox(event, field -> sub_code, field -> pdata);
            break;

        case SDLGL_TYPE_RB:
            field -> workval = sdlglfldRadioButton(event, field -> sub_code, field -> pdata);
		    break;

		case SDLGL_TYPE_MENU:
		    if (field -> workval & 0x80) {

			   field -> workval ^= field -> sub_code;
               return 1;

		    }
        default:
            return 0;
	}

    return 1;

}