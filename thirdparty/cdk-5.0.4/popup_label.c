#include <cdk_int.h>

/*
 * $Author: vegogine $
 * $Date: 2013/12/24 18:05:57 $
 * $Revision: 1.1 $
 */

/*
 * This pops up a message.
 */
void popupLabel (CDKSCREEN *screen, CDK_CSTRING2 mesg, int count)
{
   CDKLABEL *popup = 0;
   int oldCursState;
   boolean functionKey;

   /* Create the label. */
   popup = newCDKLabel (screen, CENTER, CENTER, mesg, count, TRUE, FALSE);

   oldCursState = curs_set (0);
   /* Draw it on the screen. */
   drawCDKLabel (popup, TRUE);

   /* Wait for some input. */
   keypad (popup->win, TRUE);
   getchCDKObject (ObjOf (popup), &functionKey);

   /* Kill it. */
   destroyCDKLabel (popup);

   /* Clean the screen. */
   curs_set (oldCursState);
   eraseCDKScreen (screen);
   refreshCDKScreen (screen);
}

/*
 * This pops up a message.
 */
void popupLabelAttrib (CDKSCREEN *screen, CDK_CSTRING2 mesg, int count, chtype attrib)
{
   CDKLABEL *popup = 0;
   int oldCursState;
   boolean functionKey;

   /* Create the label. */
   popup = newCDKLabel (screen, CENTER, CENTER, mesg, count, TRUE, FALSE);
   setCDKLabelBackgroundAttrib (popup, attrib);

   oldCursState = curs_set (0);
   /* Draw it on the screen. */
   drawCDKLabel (popup, TRUE);

   /* Wait for some input. */
   keypad (popup->win, TRUE);
   getchCDKObject (ObjOf (popup), &functionKey);

   /* Kill it. */
   destroyCDKLabel (popup);

   /* Clean the screen. */
   curs_set (oldCursState);
   eraseCDKScreen (screen);
   refreshCDKScreen (screen);
}