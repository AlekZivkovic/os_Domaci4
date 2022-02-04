# Operative Systems H1

Change the screen color

The goal of the homework is to modify the xv6 system to support changing the font and background color for the entire screen. Completed homework should contain the following functionalities:

● Show and hide the color selection menu.

● Navigate the menu and select an item.

● Choose a background color and letters for the screen.

## Show and hide tables

It is necessary to support the display of the color table, and allow the user to navigate through that table and select an item. The table is entered by holding down the \&lt;ALT\&gt; key, and pressing the letter combination: c o l. It should also be possible to release the \&lt;ALT\&gt; key between individual letters, as long as nothing else is pressed between individual letters. The same key combination should hide the table and return the user to normal mode. While the table is displayed, nothing but the scroll key should have an effect on the terminal. The \&lt;ALT\&gt; key should not be pressed while working with a table - only when showing and hiding it. After the table is removed from the screen, what was in that place should appear in its place before the table appeared.

## Position and layout of the table

The table is located in the upper right corner of the screen, and should have two columns: one for choosing the color of the letters, and the other for choosing the background color. Both columns have eight items - one for each base color supported in xv6: Black, Blue, Green, Aqua, Red, Purple, Yellow, White. The table should be displayed in bright white (0xf) on a black background (0x0). The table is displayed in these colors even if another color combination is selected for the rest of the terminal. The current position inside the table should be marked in inverted colors - black letters on a bright white background. In the following example, the user placed the cursor in the table on the &quot;Green&quot; item in the first column.

/---\&lt;FG\&gt;------\&lt;BG\&gt;---\
 |Black |Black |

|Blue |Blue |

|Green |Green |

|Aqua |Aqua |

|Red |Red |

|Purple |Purple |

|Yellow |Yellow |

|White |White |

\---------------------------/

## Work with a table

Navigating through the table is done using the keys: w, a, s, and d, specifically:

● w - move up.

● s - move down.

● a - move left (ie change the active column).

● d - move to the right (ie change the active column).

To select an item in the table, use the e and r keys, specifically:

● e - selects the currently selected color in its basic shape.

● r - selects the currently selected color in its lighter form.
