-- $Id: plplot.adb.cmake 7929 2007-10-14 15:42:29Z airwin $

-- Thick Ada binding to PLplot

-- Copyright (C) 2006-2007 Jerry Bauck

-- This file is part of PLplot.

-- PLplot is free software; you can redistribute it and/or modify
-- it under the terms of the GNU General Library Public License as published
-- by the Free Software Foundation; either version 2 of the License, or
-- (at your option) any later version.

-- PLplot is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU Library General Public License for more details.

-- You should have received a copy of the GNU Library General Public License
-- along with PLplot; if not, write to the Free Software
-- Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

with
    PLplot_Thin,
    PLplot_Auxiliary,
    Ada.Text_IO,
    Ada.Numerics.Long_Elementary_Functions,
    Ada.Strings.Unbounded,
    Ada.Unchecked_Conversion,
    Ada.Strings.Maps,
    Ada.Command_Line,
    System,
    System.Address_To_Access_Conversions,
    Interfaces.C.Pointers,
    Interfaces.C;
use
    PLplot_Thin,
    PLplot_Auxiliary,
    Ada.Text_IO,
    Ada.Numerics.Long_Elementary_Functions,
    Ada.Strings.Unbounded,
    Interfaces.C;

-- COMMENT THIS LINE IF YOUR COMPILER DOES NOT INCLUDE THESE 
-- DEFINITIONS, FOR EXAMPLE, IF IT IS NOT ADA 2005 WITH ANNEX G.3 COMPLIANCE.
--with Ada.Numerics.Long_Real_Arrays; use Ada.Numerics.Long_Real_Arrays;
@Ada_Is_2007_With_and_Use_Numerics@

package body PLplot is    

--------------------------------------------------------------------------------
--        High-Level subroutines for thick binding                            --
--------------------------------------------------------------------------------

    -- When asked to draw white lines on black background, do it.
    -- This is the default.
    procedure Draw_White_On_Black is
    begin
        Set_One_Color_Map_0(Black, 0,   0  ,   0);
        Set_One_Color_Map_0(White, 255, 255, 255);
    end Draw_White_On_Black;

    
    -- When asked to draw black lines on white background, reverse black and white.
    -- This might look better on anti-aliased displays.
    -- fix this Darken some colors which have low contrast on white background, e.g. Yellow.
    -- fix this Make a version that draws on white and converts _all_ colors to black for publications.
    procedure Draw_Black_On_White is
    begin
        Set_One_Color_Map_0(Black, 255, 255, 255);
        Set_One_Color_Map_0(White, 0,   0,   0);
    end Draw_Black_On_White;
    
    
    -- Set default pen width. Docs don't say, so I'll make it 1.
    -- I could make this depend on the type of outut device used.
    Default_Pen_Width : constant Integer := 1;
    procedure Set_Default_Pen_Width is
    begin
        Set_Pen_Width(Default_Pen_Width);
    end Set_Default_Pen_Width;


    -- Plotter for up to five x-y pairs and settable axis style, plot 
    -- line colors, widths, and styles, justification, zoom, and labels.
    -- Can be used directly or as part of a "simple" plotter 
    -- such as those that follow or which are made by the user.
    procedure Multiplot_Pairs
       (x1            : Real_Vector     := Dont_Plot_This;
        y1            : Real_Vector     := Dont_Plot_This;
        x2            : Real_Vector     := Dont_Plot_This;
        y2            : Real_Vector     := Dont_Plot_This;
        x3            : Real_Vector     := Dont_Plot_This;
        y3            : Real_Vector     := Dont_Plot_This;
        x4            : Real_Vector     := Dont_Plot_This;
        y4            : Real_Vector     := Dont_Plot_This;
        x5            : Real_Vector     := Dont_Plot_This;
        y5            : Real_Vector     := Dont_Plot_This;
        X_Labels      : Label_String_Array_Type := Default_Label_String_Array;
        Y_Labels      : Label_String_Array_Type := Default_Label_String_Array;
        Title_Labels  : Label_String_Array_Type := Default_Label_String_Array;
        Axis_Style    : Axis_Style_Type         := Linear_Box_Plus;
        Colors        : Color_Array_Type        := Default_Color_Array;
        Line_Widths   : Line_Width_Array_Type   := Default_Line_Width_Array;
        Line_Styles   : Line_Style_Array_Type   := Default_Line_Style_Array;
        Justification : Justification_Type      := Not_Justified;
        x_Min_Zoom    : Long_Float              := Long_Float'small;
        x_Max_Zoom    : Long_Float              := Long_Float'large;
        y_Min_Zoom    : Long_Float              := Long_Float'small;
        y_Max_Zoom    : Long_Float              := Long_Float'large) is

        x_Min, y_Min : Long_Float := Long_Float'large;
        x_Max, y_Max : Long_Float := Long_Float'small;

    begin
        -- Set or find x_Min.
        if x_Min_Zoom /= Long_Float'small then -- zoom
            x_Min := x_Min_Zoom;
        else -- Auto-scale x_Min.
            if x1'length /= 1 then
                x_Min := Long_Float'min(x_Min, Vector_Min(x1));
            end if;
            if x2'length /= 1 then
                x_Min := Long_Float'min(x_Min, Vector_Min(x2));
            end if;
            if x3'length /= 1 then
                x_Min := Long_Float'min(x_Min, Vector_Min(x3));
            end if;
            if x4'length /= 1 then
                x_Min := Long_Float'min(x_Min, Vector_Min(x4));
            end if;
            if x5'length /= 1 then
                x_Min := Long_Float'min(x_Min, Vector_Min(x5));
            end if;            
        end if; -- Set or find x_Min.
        
        -- Set or find x_Max.
        if x_Max_Zoom /= Long_Float'large then -- zoom
            x_Max := x_Max_Zoom;
        else  -- Auto-scale x_Max.
            if x1'length /= 1 then
                x_Max := Long_Float'max(x_Max, Vector_Max(x1));
            end if;
            if x2'length /= 1 then
                x_Max := Long_Float'max(x_Max, Vector_Max(x2));
            end if;
            if x3'length /= 1 then
                x_Max := Long_Float'max(x_Max, Vector_Max(x3));
            end if;
            if x4'length /= 1 then
                x_Max := Long_Float'max(x_Max, Vector_Max(x4));
            end if;
            if x5'length /= 1 then
                x_Max := Long_Float'max(x_Max, Vector_Max(x5));
            end if;
        end if; -- Set or find x_Max.

        -- Set or find y_Min.
        if y_Min_Zoom /= Long_Float'small then -- zoom
            y_Min := y_Min_Zoom;
        else -- Auto-scale y_Min.
            if y1'length /= 1 then
                y_Min := Long_Float'min(y_Min, Vector_Min(y1));
            end if;
            if y2'length /= 1 then
                y_Min := Long_Float'min(y_Min, Vector_Min(y2));
            end if;
            if y3'length /= 1 then
                y_Min := Long_Float'min(y_Min, Vector_Min(y3));
            end if;
            if y4'length /= 1 then
                y_Min := Long_Float'min(y_Min, Vector_Min(y4));
            end if;
            if y5'length /= 1 then
                y_Min := Long_Float'min(y_Min, Vector_Min(y5));
            end if;            
        end if; -- Set or find y_Min.
        
        -- Set or find y_Max.
        if y_Max_Zoom /= Long_Float'large then -- zoom
            y_Max := y_Max_Zoom;
        else  -- Auto-scale y_Max.
            if y1'length /= 1 then
                y_Max := Long_Float'max(y_Max, Vector_Max(y1));
            end if;
            if y2'length /= 1 then
                y_Max := Long_Float'max(y_Max, Vector_Max(y2));
            end if;
            if y3'length /= 1 then
                y_Max := Long_Float'max(y_Max, Vector_Max(y3));
            end if;
            if y4'length /= 1 then
                y_Max := Long_Float'max(y_Max, Vector_Max(y4));
            end if;
            if y5'length /= 1 then
                y_Max := Long_Float'max(y_Max, Vector_Max(y5));
            end if;
        end if; -- Set or find x_Max.


        -- Set environment and its color.
        Set_Pen_Color(White);
--        Set_Environment_Clear_Subpage(x_Min, x_Max, y_Min, y_Max, Justification, Axis_Style);
        Set_Environment(x_Min, x_Max, y_Min, y_Max, Justification, Axis_Style);
        
        if x1'length /= 1 and y1'length /= 1 then
            Write_Labels(To_String(X_Labels(1)), To_String(Y_Labels(1)), To_String(Title_Labels(1)));
            Set_Pen_Color(Colors(1));
            Select_Line_Style(Line_Styles(1));
            Draw_Curve(x1, y1);
        end if;

        if x2'length /= 1 and y2'length /= 1 then
            Write_Labels(To_String(X_Labels(2)), To_String(Y_Labels(2)), To_String(Title_Labels(2)));
            Set_Pen_Color(Colors(2));
            Select_Line_Style(Line_Styles(2));
            Draw_Curve(x2, y2);
        end if;

        if x3'length /= 1 and y3'length /= 1 then
            Write_Labels(To_String(X_Labels(3)), To_String(Y_Labels(3)), To_String(Title_Labels(3)));
            Set_Pen_Color(Colors(3));
            Select_Line_Style(Line_Styles(3));
            Draw_Curve(x3, y3);
        end if;

        if x4'length /= 1 and y4'length /= 1 then
            Write_Labels(To_String(X_Labels(4)), To_String(Y_Labels(4)), To_String(Title_Labels(4)));
            Set_Pen_Color(Colors(4));
            Select_Line_Style(Line_Styles(4));
            Draw_Curve(x4, y4);
        end if;

        if x5'length /= 1 and y5'length /= 1 then
            Write_Labels(To_String(X_Labels(5)), To_String(Y_Labels(5)), To_String(Title_Labels(5)));
            Set_Pen_Color(Colors(5));
            Select_Line_Style(Line_Styles(5));
            Draw_Curve(x5, y5);
        end if;
        
        Set_Pen_Color(White);
        Set_Default_Pen_Width;
        Select_Line_Style(1); --solid
    end Multiplot_Pairs;


--------- Simple plotters requiring minimal arguments -----

    
    -- Simple plotter for single x array and multiple y arrays
    procedure Simple_Plot 
       (x  : Real_Vector;
        y1 : Real_Vector := Dont_Plot_This;
        y2 : Real_Vector := Dont_Plot_This;
        y3 : Real_Vector := Dont_Plot_This;
        y4 : Real_Vector := Dont_Plot_This;
        y5 : Real_Vector := Dont_Plot_This;
        X_Label     : String := To_String(Default_Label_String);
        Y_Label     : String := To_String(Default_Label_String);
        Title_Label : String := To_String(Default_Label_String)) is

        X_Label_String_Array     : Label_String_Array_Type := Default_Label_String_Array;
        Y_Label_String_Array     : Label_String_Array_Type := Default_Label_String_Array;
        Title_Label_String_Array : Label_String_Array_Type := Default_Label_String_Array;

    begin
        X_Label_String_Array(1)     := TUB(X_Label);     -- First slot only; others not used.
        Y_Label_String_Array(1)     := TUB(Y_Label);     -- First slot only; others not used.
        Title_Label_String_Array(1) := TUB(Title_Label); -- First slot only; others not used.

        Multiplot_Pairs(x, y1, x, y2, x, y3, x, y4, x, y5, 
            X_Labels     => X_Label_String_Array,
            Y_Labels     => Y_Label_String_Array,
            Title_Labels => Title_Label_String_Array,
            Axis_Style   => Linear_Major_Grid);
    end Simple_Plot;
    
    
    -- Simple log x plotter for single x array and multiple y arrays
    -- fix this: Automatically skip zero-valued abscissa; place marker at the
    -- left-hand side of the plot at the ordinate of the deleted point.
    procedure Simple_Plot_Log_X
       (x  : Real_Vector;
        y1 : Real_Vector := Dont_Plot_This;
        y2 : Real_Vector := Dont_Plot_This;
        y3 : Real_Vector := Dont_Plot_This;
        y4 : Real_Vector := Dont_Plot_This;
        y5 : Real_Vector := Dont_Plot_This;
        X_Label     : String := To_String(Default_Label_String);
        Y_Label     : String := To_String(Default_Label_String);
        Title_Label : String := To_String(Default_Label_String);
        Log_Base : Long_Float := 10.0) is -- Should this default to e?

        X_Label_String_Array     : Label_String_Array_Type := Default_Label_String_Array;
        Y_Label_String_Array     : Label_String_Array_Type := Default_Label_String_Array;
        Title_Label_String_Array : Label_String_Array_Type := Default_Label_String_Array;

        x_Log : Real_Vector(x'range);

    begin
        X_Label_String_Array(1)     := TUB(X_Label);     -- First slot only; others not used.
        Y_Label_String_Array(1)     := TUB(Y_Label);     -- First slot only; others not used.
        Title_Label_String_Array(1) := TUB(Title_Label); -- First slot only; others not used.

        for i in x_Log'range loop
            x_Log(i) := Log(x(i), Log_Base);
        end loop;        
        Multiplot_Pairs(x_Log, y1, x_Log, y2, x_Log, y3, x_Log, y4, x_Log, y5, 
            X_Labels     => X_Label_String_Array,
            Y_Labels     => Y_Label_String_Array,
            Title_Labels => Title_Label_String_Array,
            Axis_Style   => Log_X_Minor_Grid);
    end Simple_Plot_Log_X;
    
    
    -- Simple log y plotter for multiple x arrays and single y array
    -- fix this: Automatically skip zero-valued ordinate; place marker at the
    -- bottom of the plot at the abscissa of the deleted point.
    procedure Simple_Plot_Log_Y
       (x1 : Real_Vector := Dont_Plot_This;
        y  : Real_Vector := Dont_Plot_This; -- Beware of argument order.
        x2 : Real_Vector := Dont_Plot_This;
        x3 : Real_Vector := Dont_Plot_This;
        x4 : Real_Vector := Dont_Plot_This;
        x5 : Real_Vector := Dont_Plot_This;
        X_Label     : String := To_String(Default_Label_String);
        Y_Label     : String := To_String(Default_Label_String);
        Title_Label : String := To_String(Default_Label_String);
        Log_Base : Long_Float := 10.0) is -- Should this default to e?

        X_Label_String_Array     : Label_String_Array_Type := Default_Label_String_Array;
        Y_Label_String_Array     : Label_String_Array_Type := Default_Label_String_Array;
        Title_Label_String_Array : Label_String_Array_Type := Default_Label_String_Array;

        y_Log : Real_Vector(y'range);

    begin
        X_Label_String_Array(1)     := TUB(X_Label);     -- First slot only; others not used.
        Y_Label_String_Array(1)     := TUB(Y_Label);     -- First slot only; others not used.
        Title_Label_String_Array(1) := TUB(Title_Label); -- First slot only; others not used.

        for i in y_Log'range loop
            y_Log(i) := Log(y(i), Log_Base);
        end loop;        
        Multiplot_Pairs(x1, y_Log, x2, y_Log, x3, y_Log, x4, y_Log, x5, y_Log, 
            X_Labels     => X_Label_String_Array,
            Y_Labels     => Y_Label_String_Array,
            Title_Labels => Title_Label_String_Array,
            Axis_Style   => Log_Y_Minor_Grid);
    end Simple_Plot_Log_Y;
    
    
    -- Simple log x - log y plotter
    procedure Simple_Plot_Log_XY
       (x, y        : Real_Vector;
        X_Label     : String := To_String(Default_Label_String);
        Y_Label     : String := To_String(Default_Label_String);
        Title_Label : String := To_String(Default_Label_String);
        x_Log_Base, y_Log_Base : Long_Float := 10.0) is -- Should this default to e?

        X_Label_String_Array     : Label_String_Array_Type := Default_Label_String_Array;
        Y_Label_String_Array     : Label_String_Array_Type := Default_Label_String_Array;
        Title_Label_String_Array : Label_String_Array_Type := Default_Label_String_Array;

        x_Log : Real_Vector(x'range);
        y_Log : Real_Vector(y'range);
    begin
        X_Label_String_Array(1)     := TUB(X_Label);     -- First slot only; others not used.
        Y_Label_String_Array(1)     := TUB(Y_Label);     -- First slot only; others not used.
        Title_Label_String_Array(1) := TUB(Title_Label); -- First slot only; others not used.

        for i in x_Log'range loop
            x_Log(i) := Log(x(i), x_Log_Base);
            y_Log(i) := Log(y(i), y_Log_Base);
        end loop;        
        Multiplot_Pairs(x_Log, y_Log, 
            X_Labels     => X_Label_String_Array,
            Y_Labels     => Y_Label_String_Array,
            Title_Labels => Title_Label_String_Array,
            Axis_Style   => Log_XY_Minor_Grid);
    end Simple_Plot_Log_XY;
    
    
    -- Simple plotter for multiple x-y arrays specified pairwise.
    procedure Simple_Plot_Pairs
       (x1 : Real_Vector := Dont_Plot_This;
        y1 : Real_Vector := Dont_Plot_This;
        x2 : Real_Vector := Dont_Plot_This;
        y2 : Real_Vector := Dont_Plot_This;
        x3 : Real_Vector := Dont_Plot_This;
        y3 : Real_Vector := Dont_Plot_This;
        x4 : Real_Vector := Dont_Plot_This;
        y4 : Real_Vector := Dont_Plot_This;
        x5 : Real_Vector := Dont_Plot_This;
        y5 : Real_Vector := Dont_Plot_This;
        X_Label     : String := To_String(Default_Label_String);
        Y_Label     : String := To_String(Default_Label_String);
        Title_Label : String := To_String(Default_Label_String)) is

        X_Label_String_Array     : Label_String_Array_Type := Default_Label_String_Array;
        Y_Label_String_Array     : Label_String_Array_Type := Default_Label_String_Array;
        Title_Label_String_Array : Label_String_Array_Type := Default_Label_String_Array;
    begin
        X_Label_String_Array(1)     := TUB(X_Label);     -- First slot only; others not used.
        Y_Label_String_Array(1)     := TUB(Y_Label);     -- First slot only; others not used.
        Title_Label_String_Array(1) := TUB(Title_Label); -- First slot only; others not used.

        Multiplot_Pairs(x1, y1, x2, y2, x3, y3, x4, y4, x5, y5, 
            X_Labels     => X_Label_String_Array,
            Y_Labels     => Y_Label_String_Array,
            Title_Labels => Title_Label_String_Array,
            Axis_Style   => Linear_Major_Grid);
    end Simple_Plot_Pairs;


--------- Plotter requiring somewhat more arguments ------

    -- Single plotter with flexible attributes
    -- Similar to Multiplot_Pairs except single trace and no attribute arrays.
    procedure Single_Plot
       (x, y          : Real_Vector;
        X_Label       : String             := To_String(Default_Label_String);
        Y_Label       : String             := To_String(Default_Label_String);
        Title_Label   : String             := To_String(Default_Label_String);
        Axis_Style    : Axis_Style_Type    := Linear_Major_Grid;
        Color         : Plot_Color_Type    := Red;
        Line_Width    : Integer            := 1;
        Line_Style    : Line_Style_Type    := 1;
        Justification : Justification_Type := Not_Justified;
        x_Min_Zoom    : Long_Float         := Long_Float'small;
        x_Max_Zoom    : Long_Float         := Long_Float'large;
        y_Min_Zoom    : Long_Float         := Long_Float'small;
        y_Max_Zoom    : Long_Float         := Long_Float'large) is
    
        X_Label_String_Array     : Label_String_Array_Type := Default_Label_String_Array;
        Y_Label_String_Array     : Label_String_Array_Type := Default_Label_String_Array;
        Title_Label_String_Array : Label_String_Array_Type := Default_Label_String_Array;
        Color_Array              : Color_Array_Type        := Default_Color_Array;
        Line_Width_Array         : Line_Width_Array_Type   := Default_Line_Width_Array;
        Line_Style_Array         : Line_Style_Array_Type   := Default_Line_Style_Array;

    begin
        X_Label_String_Array(1)     := TUB(X_Label);     -- First slot only; others not used.
        Y_Label_String_Array(1)     := TUB(Y_Label);     -- First slot only; others not used.
        Title_Label_String_Array(1) := TUB(Title_Label); -- First slot only; others not used.
        Color_Array(1)              := Color;
        Line_Width_Array(1)         := Line_Width;  -- First slot only; others not used.
        Line_Style_Array(1)         := Line_Style;  -- First slot only; others not used.

        -- Process arrays if log plot is indicated.
        -- The declare blocks save memory and time for non-log plots.

        -- Log x.
        if Axis_Style in Log_X_Box_Plus..Log_X_Minor_Grid then
            declare
                x_Log : Real_Vector(x'range);
            begin
                for i in x_Log'range loop
                    x_Log(i) := Log(x(i), 10.0);
                end loop;

                Multiplot_Pairs(
                    x_Log, y, 
                    X_Labels      => X_Label_String_Array,
                    Y_Labels      => Y_Label_String_Array,
                    Title_Labels  => Title_Label_String_Array,
                    Axis_Style    => Axis_Style, 
                    Colors        => Color_Array, 
                    Line_Widths   => Line_Width_Array, 
                    Line_Styles   => Line_Style_Array, 
                    Justification => Justification, 
                    x_Min_Zoom    => x_Min_Zoom, 
                    x_Max_Zoom    => x_Max_Zoom, 
                    y_Min_Zoom    => y_Min_Zoom, 
                    y_Max_Zoom    => y_Max_Zoom);
            end; -- declare
        end if; -- log x

        -- Log y
        if Axis_Style in Log_Y_Box_Plus..Log_Y_Minor_Grid then
            declare
                y_Log : Real_Vector(y'range);
            begin
                for i in y_Log'range loop
                    y_Log(i) := Log(y(i), 10.0);
                end loop;
                Multiplot_Pairs(
                    x, y_Log, 
                    X_Labels      => X_Label_String_Array,
                    Y_Labels      => Y_Label_String_Array,
                    Title_Labels  => Title_Label_String_Array,
                    Axis_Style    => Axis_Style, 
                    Colors        => Color_Array, 
                    Line_Widths   => Line_Width_Array, 
                    Line_Styles   => Line_Style_Array, 
                    Justification => Justification, 
                    x_Min_Zoom    => x_Min_Zoom, 
                    x_Max_Zoom    => x_Max_Zoom, 
                    y_Min_Zoom    => y_Min_Zoom, 
                    y_Max_Zoom    => y_Max_Zoom);
            end; -- declare
        end if; -- log y

        -- Log x and log y
        if Axis_Style in Log_XY_Box_Plus..Log_XY_Minor_Grid then
            declare
                x_Log : Real_Vector(x'range);
                y_Log : Real_Vector(y'range);
            begin
                for i in x_Log'range loop
                    x_Log(i) := Log(x(i), 10.0);
                    y_Log(i) := Log(y(i), 10.0);
                end loop;
                Multiplot_Pairs(
                    x_Log, y_Log, 
                    X_Labels      => X_Label_String_Array,
                    Y_Labels      => Y_Label_String_Array,
                    Title_Labels  => Title_Label_String_Array,
                    Axis_Style    => Axis_Style, 
                    Colors        => Color_Array, 
                    Line_Widths   => Line_Width_Array, 
                    Line_Styles   => Line_Style_Array, 
                    Justification => Justification, 
                    x_Min_Zoom    => x_Min_Zoom, 
                    x_Max_Zoom    => x_Max_Zoom, 
                    y_Min_Zoom    => y_Min_Zoom, 
                    y_Max_Zoom    => y_Max_Zoom);
            end; -- declare
        end if; -- log x and log y

        -- Linear plot is indicated.
        if Axis_Style in No_Box..Linear_Minor_Grid then
            Multiplot_Pairs(
                x, y, 
                X_Labels      => X_Label_String_Array,
                Y_Labels      => Y_Label_String_Array,
                Title_Labels  => Title_Label_String_Array,
                Axis_Style    => Axis_Style, 
                Colors        => Color_Array, 
                Line_Widths   => Line_Width_Array, 
                Line_Styles   => Line_Style_Array, 
                Justification => Justification, 
                x_Min_Zoom    => x_Min_Zoom, 
                x_Max_Zoom    => x_Max_Zoom, 
                y_Min_Zoom    => y_Min_Zoom, 
                y_Max_Zoom    => y_Max_Zoom);
        end if; -- Linear plot
    end Single_Plot;


--------- Simple Contour Plotter ------

    -- Place-holder for the transformation data which is not used here.
    Transformation_Data : aliased Transformation_Data_Type 
        (x_Length_Minus_1 => 2, y_Length_Minus_1 => 2, z_Length_Minus_1 => 2);

    procedure Simple_Contour
       (z             : Real_Matrix;
        Number_Levels : Integer := 10;
        X_Label       : String  := To_String(Default_Label_String);
        Y_Label       : String  := To_String(Default_Label_String);
        Title_Label   : String  := To_String(Default_Label_String)) is
       
        Contour_Levels : Real_Vector (0 .. Number_Levels);
        Transformation_Procedure_Pointer : Transformation_Procedure_Pointer_Type;

    begin
        -- Fill up the dummy transformation data with something so we don't get warnings.
        for i in Transformation_Data.xg'range loop -- use shorthand for .all on pointer
            Transformation_Data.xg(i) := 0.0;
        end loop;
        for i in Transformation_Data.yg'range loop
            Transformation_Data.yg(i) := 0.0;
        end loop;
        for i in Transformation_Data.zg'range loop
            Transformation_Data.zg(i) := 0.0;
        end loop;

        -- Point to the do-nothing transformation procedure.
        Transformation_Procedure_Pointer := PLplot_Thin.pltr0'access;

        -- Fill the contour vector with some levels.
        Calculate_Contour_Levels(Contour_Levels, Matrix_Min(z), Matrix_Max(z));
        
        Advance_To_Subpage(Next_Subpage);
        Set_Viewport_Normalized(0.1, 0.9, 0.1, 0.9);
        Set_Viewport_World(1.0, 35.0, 1.0, 46.0); -- fix
        Set_Pen_Color(White);
        Box_Around_Viewport("bcnst", 0.0, 0, "bcnstv", 0.0, 0);
        Set_Pen_Color(White);
        Write_Labels(X_Label, Y_Label, Title_Label);
        Set_Pen_Color(White);
        Set_Contour_Label_Parameters(0.008, 0.6, 0.1, True);
        Contour_Plot(z, z'First(1), z'Last(1), z'First(2), z'Last(2), 
            Contour_Levels, Transformation_Procedure_Pointer, Transformation_Data);          
    end Simple_Contour;


--------- Simple 3D Mesh Plotter ------

    procedure Simple_Mesh_3D
       (x, y     : Real_Vector; -- data definition points 
        z        : Real_Matrix; -- z(x, y) = z(x(i), y(j))
        x_Min    : Long_Float := 0.0; -- user coordinate limits
        x_Max    : Long_Float := 0.0;
        y_Min    : Long_Float := 0.0;
        y_Max    : Long_Float := 0.0;
        Altitude : Long_Float := 30.0; -- viewing elevation angle in degrees
        Azimuth  : Long_Float := 30.0; -- viewing azimuth in degrees
        X_Label  : String := "x";
        Y_Label  : String := "y";
        Z_Label  : String := "z") is

        x_Min_Local, x_Max_Local, y_Min_Local, y_Max_Local : Long_Float;
        
    begin
        -- Set min and max for x and y if they are not the defaults.
        if x_Min = 0.0 and x_Max = 0.0 then -- override
            x_Min_Local := Vector_Min(x);
            x_Max_Local := Vector_Max(x);
        else
            x_Min_Local := x_Min;
            x_Max_Local := x_Max;
        end if;
        
        if y_Min = 0.0 and y_Max = 0.0 then -- override
            y_Min_Local := Vector_Min(y);
            y_Max_Local := Vector_Max(y);
        else
            y_Min_Local := y_Min;
            y_Max_Local := y_Max;
        end if;
        
        Quick_Set_Color_Map_1(Blue_Green_Red); -- no way to restore after doing this
        Advance_To_Subpage(Next_Subpage);
        Set_Pen_Color(White);
        Set_Viewport_Normalized(0.0, 1.0, 0.0, 1.0);
        Set_Viewport_World(-0.9, 0.9, -0.8, 1.5);
        Set_Up_3D(1.0, 1.0, 1.0, x_Min_Local, x_Max_Local, y_Min_Local, y_Max_Local, 
            Matrix_Min(z), Matrix_Max(z), Altitude, Azimuth); -- plw3d
        Box_Around_Viewport_3D("bnstu", X_Label, 0.0, 0, 
            "bnstu", Y_Label, 0.0, 0, 
            "bcdmnstuv", Z_Label, 0.0, 0); -- plbox3
        Mesh_3D(x, y, z, Lines_Parallel_To_X_And_Y + Magnitude_Color); -- plmesh
    end Simple_Mesh_3D;


--------- Simple 3D Surface Plotter ------

    procedure Simple_Surface_3D
       (x, y     : Real_Vector; -- data definition points 
        z        : Real_Matrix; -- z(x, y) = z(x(i), y(j))
        x_Min    : Long_Float := 0.0; -- user coordinate limits
        x_Max    : Long_Float := 0.0;
        y_Min    : Long_Float := 0.0;
        y_Max    : Long_Float := 0.0;
        Altitude : Long_Float := 30.0; -- viewing elevation angle in degrees
        Azimuth  : Long_Float := 30.0; -- viewing azimuth in degrees
        X_Label  : String := "x";
        Y_Label  : String := "y";
        Z_Label  : String := "z") is

        x_Min_Local, x_Max_Local, y_Min_Local, y_Max_Local : Long_Float;
        Contour_Levels_Dummy : Real_Vector(0..1) := (others => 0.0);
        
    begin
        -- Set min and max for x and y if they are not the defaults.
        if x_Min = 0.0 and x_Max = 0.0 then -- override
            x_Min_Local := Vector_Min(x);
            x_Max_Local := Vector_Max(x);
        else
            x_Min_Local := x_Min;
            x_Max_Local := x_Max;
        end if;
        
        if y_Min = 0.0 and y_Max = 0.0 then -- override
            y_Min_Local := Vector_Min(y);
            y_Max_Local := Vector_Max(y);
        else
            y_Min_Local := y_Min;
            y_Max_Local := y_Max;
        end if;
        
        Quick_Set_Color_Map_1(Blue_Green_Red); -- no way to restore after doing this
        Advance_To_Subpage(Next_Subpage);
        Set_Pen_Color(White);
        Set_Viewport_Normalized(0.0, 1.0, 0.0, 1.0);
        Set_Viewport_World(-0.9, 0.9, -0.8, 1.5);
        Set_Up_3D(1.0, 1.0, 1.0, x_Min_Local, x_Max_Local, y_Min_Local, y_Max_Local, 
            Matrix_Min(z), Matrix_Max(z), Altitude, Azimuth); -- plw3d
        Box_Around_Viewport_3D("bnstu", X_Label, 0.0, 0, 
            "bnstu", Y_Label, 0.0, 0, 
            "bcdmnstuv", Z_Label, 0.0, 0); -- plbox3
        Mesh_3D(x, y, z, Lines_Parallel_To_X_And_Y + Magnitude_Color); -- plmesh
        Shaded_Surface_3D(x, y, z, Magnitude_Color, Contour_Levels_Dummy);
    end Simple_Surface_3D;
        


--------- Simple color table manipulatons -----

    -- Things for manipulating color map 0 --

    -- Make a snapshot of color map 0 for possible later full or partial restoration.
    -- This is automatically called at package initialization with results stored
    -- in Default_Red_Components, Default_Green_Components, Default_Blue_Components.
    procedure Make_Snapshot_Of_Color_Map_0 
       (Reds, Greens, Blues : out Integer_Array_1D) is
    begin
        for i in Reds'range loop
            Get_Color_RGB(i, Reds(i), Greens(i), Blues(i));
        end loop;
    end Make_Snapshot_Of_Color_Map_0;
    
    
    -- Restore an arbitray snapshot of color map 0.
    procedure Restore_Snapshot_Of_Color_Map_0
       (Reds, Greens, Blues : Integer_Array_1D) is
    begin
        Set_Color_Map_0(Reds, Greens, Blues);
    end Restore_Snapshot_Of_Color_Map_0;
    
    
    -- Restore the default colors of color map 0 taken as a snapshot at initialization.
    procedure Restore_Default_Snapshot_Of_Color_Map_0 is
    begin
        Set_Number_Of_Colors_Map_0(Number_Of_Default_Colors);
        Set_Color_Map_0(Default_Red_Components, Default_Green_Components, Default_Blue_Components);
    end Restore_Default_Snapshot_Of_Color_Map_0;
    
    
    -- Functions which correspond to the default colors of color map 0. Calling
    -- one of these (1) resets the corresponding slot in color map 0 to its 
    -- default value, and (2) returns the correct integer value for the default 
    -- color specified. Thus, using Set_Pen_Color(Reset_Red) instead of 
    -- Set_Pen_Color(Red) guarantees that the color will be set to Red even if 
    -- there have been prior manipulations of color 1.
    
    function Reset_Black return Integer is
    begin
        Set_One_Color_Map_0(0, Default_Red_Components(0), Default_Green_Components(0), Default_Blue_Components(0));
        return 0;
    end Reset_Black;
    
    function Reset_Red return Integer is
    begin
        Set_One_Color_Map_0(1, Default_Red_Components(1), Default_Green_Components(1), Default_Blue_Components(1));
        return 1;
    end Reset_Red;
    
    function Reset_Yellow return Integer is
    begin
        Set_One_Color_Map_0(2, Default_Red_Components(2), Default_Green_Components(2), Default_Blue_Components(2));
        return 2;
    end Reset_Yellow;
    
    function Reset_Green return Integer is
    begin
        Set_One_Color_Map_0(3, Default_Red_Components(3), Default_Green_Components(3), Default_Blue_Components(3));
        return 3;
    end Reset_Green;
    
    function Reset_Aquamarine return Integer is
    begin
        Set_One_Color_Map_0(4, Default_Red_Components(4), Default_Green_Components(4), Default_Blue_Components(4));
        return 4;
    end Reset_Aquamarine;
    
    function Reset_Pink return Integer is
    begin
        Set_One_Color_Map_0(5, Default_Red_Components(5), Default_Green_Components(5), Default_Blue_Components(5));
        return 5;
    end Reset_Pink;
    
    function Reset_Wheat return Integer is
    begin
        Set_One_Color_Map_0(6, Default_Red_Components(6), Default_Green_Components(6), Default_Blue_Components(6));
        return 6;
    end Reset_Wheat;
    
    function Reset_Grey return Integer is
    begin
        Set_One_Color_Map_0(7, Default_Red_Components(7), Default_Green_Components(7), Default_Blue_Components(7));
        return 7;
    end Reset_Grey;
    
    function Reset_Brown return Integer is
    begin
        Set_One_Color_Map_0(8, Default_Red_Components(8), Default_Green_Components(8), Default_Blue_Components(8));
        return 8;
    end Reset_Brown;
    
    function Reset_Blue return Integer is
    begin
        Set_One_Color_Map_0(9, Default_Red_Components(9), Default_Green_Components(9), Default_Blue_Components(9));
        return 9;
    end Reset_Blue;
    
    function Reset_BlueViolet return Integer is
    begin
        Set_One_Color_Map_0(10, Default_Red_Components(10), Default_Green_Components(10), Default_Blue_Components(10));
        return 10;
    end Reset_BlueViolet;
    
    function Reset_Cyan return Integer is
    begin
        Set_One_Color_Map_0(11, Default_Red_Components(11), Default_Green_Components(11), Default_Blue_Components(11));
        return 11;
    end Reset_Cyan;
    
    function Reset_Turquoise return Integer is
    begin
        Set_One_Color_Map_0(12, Default_Red_Components(12), Default_Green_Components(12), Default_Blue_Components(12));
        return 12;
    end Reset_Turquoise;
    
    function Reset_Magenta return Integer is
    begin
        Set_One_Color_Map_0(13, Default_Red_Components(13), Default_Green_Components(13), Default_Blue_Components(13));
        return 13;
    end Reset_Magenta;
    
    function Reset_Salmon return Integer is
    begin
        Set_One_Color_Map_0(14, Default_Red_Components(14), Default_Green_Components(14), Default_Blue_Components(14));
        return 14;
    end Reset_Salmon;
    
    function Reset_White return Integer is
    begin
        Set_One_Color_Map_0(15, Default_Red_Components(15), Default_Green_Components(15), Default_Blue_Components(15));
        return 15;
    end Reset_White;


    -- Things for manipulating color map 1 --

    -- Quick application of pre-fabricated color schemes to color map 1.
    procedure Quick_Set_Color_Map_1(Color_Theme : Color_Themes_For_Map_1_Type) is
    
        Controls_3 : Real_Vector (0..2); -- 3 control points
        Controls_4 : Real_Vector (0..3); -- 4 control points
        Red_3, Green_3, Blue_3 : Real_Vector (0..2); -- define 3
        Red_4, Green_4, Blue_4 : Real_Vector (0..3); -- define 4
        Hue_3, Lightness_3, Saturation_3 : Real_Vector (0..2); -- define 3
        Hue_4, Lightness_4, Saturation_4 : Real_Vector (0..3); -- define 4
        Reverse_Hue_3 : Boolean_Array_1D (0..2);
        Reverse_Hue_4 : Boolean_Array_1D (0..3);
    begin
        Set_Number_Of_Colors_In_Color_Map_1(256);

        Controls_3(0) := 0.0;
        Controls_3(1) := 0.5;
        Controls_3(2) := 1.0;
        
        Controls_4(0) := 0.0;
        Controls_4(1) := 0.5; -- allow a "bend" at the mid-point
        Controls_4(2) := 0.5; -- allow a "bend" at the mid-point
        Controls_4(3) := 1.0;

        -- Initialize everything, even unused stuff.
        Red_3(0)   := 0.0;
        Red_3(1)   := 0.0;
        Red_3(2)   := 0.0;
        Green_3(0) := 0.0;
        Green_3(1) := 0.0;
        Green_3(2) := 0.0;
        Blue_3(0)  := 0.0;
        Blue_3(1)  := 0.0;
        Blue_3(2)  := 0.0;

        Red_4(0)   := 0.0;
        Red_4(1)   := 0.0;
        Red_4(2)   := 0.0;
        Red_4(3)   := 0.0;
        Green_4(0) := 0.0;
        Green_4(1) := 0.0;
        Green_4(2) := 0.0;
        Green_4(3) := 0.0;
        Blue_4(0)  := 0.0;
        Blue_4(1)  := 0.0;
        Blue_4(2)  := 0.0;
        Blue_4(3)  := 0.0;

        Hue_3(0)        := 0.0;
        Hue_3(1)        := 0.0;
        Hue_3(2)        := 0.0;
        Lightness_3(0)  := 0.0;
        Lightness_3(1)  := 0.0;
        Lightness_3(2)  := 0.0;
        Saturation_3(0) := 0.0;
        Saturation_3(1) := 0.0;
        Saturation_3(2) := 0.0;

        Hue_4(0)        := 0.0;
        Hue_4(1)        := 0.0;
        Hue_4(2)        := 0.0;
        Hue_4(3)        := 0.0;
        Lightness_4(0)  := 0.0;
        Lightness_4(1)  := 0.0;
        Lightness_4(2)  := 0.0;
        Lightness_4(3)  := 0.0;
        Saturation_4(0) := 0.0;
        Saturation_4(1) := 0.0;
        Saturation_4(2) := 0.0;
        Saturation_4(3) := 0.0;

        case Color_Theme is
            when Gray =>
                begin
                    Hue_3(0) := 0.0;
                    Hue_3(1) := 0.0;
                    Hue_3(2) := 0.0;
                    Lightness_3(0) := 0.0;
                    Lightness_3(1) := 0.5;
                    Lightness_3(2) := 1.0;
                    Saturation_3(0) := 0.0;
                    Saturation_3(1) := 0.0;
                    Saturation_3(2) := 0.0;
                    Reverse_Hue_3(0) := False;
                    Reverse_Hue_3(1) := False;
                    Reverse_Hue_3(2) := False;
                    Set_Color_Map_1_Piecewise(HLS, Controls_3, Hue_3, Lightness_3, Saturation_3, Reverse_Hue_3);
                end;
            when Blue_Green_Red =>
                begin
                    Blue_3(0) := 1.0;
                    Blue_3(1) := 0.0;
                    Blue_3(2) := 0.0;
                    Green_3(0) := 0.0;
                    Green_3(1) := 1.0;
                    Green_3(2) := 0.0;
                    Red_3(0) := 0.0;
                    Red_3(1) := 0.0;
                    Red_3(2) := 1.0;
                    Reverse_Hue_3(0) := False;
                    Reverse_Hue_3(1) := False;
                    Reverse_Hue_3(2) := False;
                    Set_Color_Map_1_Piecewise(RGB, Controls_3, Red_3, Green_3, Blue_3, Reverse_Hue_3);
                end;
            when Red_Green_Blue =>
                begin
                    Blue_3(0) := 0.0;
                    Blue_3(1) := 0.0;
                    Blue_3(2) := 1.0;
                    Green_3(0) := 0.0;
                    Green_3(1) := 1.0;
                    Green_3(2) := 0.0;
                    Red_3(0) := 1.0;
                    Red_3(1) := 0.0;
                    Red_3(2) := 0.0;
                    Reverse_Hue_3(0) := False;
                    Reverse_Hue_3(1) := False;
                    Reverse_Hue_3(2) := False;
                    Set_Color_Map_1_Piecewise(RGB, Controls_3, Red_3, Green_3, Blue_3, Reverse_Hue_3);
                end;
            when Red_Cyan_Blue =>
                begin
                    Hue_3(0) := 360.0;
                    Hue_3(1) := 300.0;
                    Hue_3(2) := 240.0;
                    Saturation_3(0) := 1.0;
                    Saturation_3(1) := 1.0;
                    Saturation_3(2) := 1.0;
                    Lightness_3(0) := 0.5;
                    Lightness_3(1) := 0.5;
                    Lightness_3(2) := 0.5;
                    Reverse_Hue_3(0) := False;
                    Reverse_Hue_3(1) := False;
                    Reverse_Hue_3(2) := False;
                    Set_Color_Map_1_Piecewise(HLS, Controls_3, Hue_3, Lightness_3, Saturation_3, Reverse_Hue_3);
                end;
            when Blue_Black_Red =>
                begin
                    Hue_4(0) := 240.0; -- Blue
                    Hue_4(1) := 240.0; -- Blue
                    Hue_4(2) := 0.0;   -- Red
                    Hue_4(3) := 0.0;   -- Red
                    Saturation_4(0) := 1.0;
                    Saturation_4(1) := 1.0;
                    Saturation_4(2) := 1.0;
                    Saturation_4(3) := 1.0;
                    Lightness_4(0) := 0.5; -- Apparently different from Brightness
                    Lightness_4(1) := 0.0;
                    Lightness_4(2) := 0.0;
                    Lightness_4(3) := 0.5;
                    Reverse_Hue_4(0) := False;
                    Reverse_Hue_4(1) := False;
                    Reverse_Hue_4(2) := False;
                    Reverse_Hue_4(3) := False;
                    Set_Color_Map_1_Piecewise(HLS, Controls_4, Hue_4, Lightness_4, Saturation_4, Reverse_Hue_4);
                end;
            when Red_Blue_Green =>
                begin
                    Hue_3(0) := 360.0; -- red
                    Hue_3(1) := 210.0; -- blue
                    Hue_3(2) := 120.0;  -- green
                    Lightness_3(0) := 0.5;
                    Lightness_3(1) := 0.5;
                    Lightness_3(2) := 0.5;
                    Saturation_3(0) := 1.0;
                    Saturation_3(1) := 1.0;
                    Saturation_3(2) := 1.0;
                    Reverse_Hue_3(0) := False;
                    Reverse_Hue_3(1) := False;
                    Reverse_Hue_3(2) := False;
                    Set_Color_Map_1_Piecewise(HLS, Controls_3, Hue_3, Lightness_3, Saturation_3, Reverse_Hue_3);
                end;
            when Red_Yellow =>
                begin
                    Hue_3(0) := 0.0;  -- red
                    Hue_3(1) := 30.0; -- orange
                    Hue_3(2) := 60.0; -- yellow
                    Lightness_3(0) := 0.5;
                    Lightness_3(1) := 0.5;
                    Lightness_3(2) := 0.5;
                    Saturation_3(0) := 1.0;
                    Saturation_3(1) := 1.0;
                    Saturation_3(2) := 1.0;
                    Reverse_Hue_3(0) := False;
                    Reverse_Hue_3(1) := False;
                    Reverse_Hue_3(2) := False;
                    Set_Color_Map_1_Piecewise(HLS, Controls_3, Hue_3, Lightness_3, Saturation_3, Reverse_Hue_3);
                end;
        end case;
    end Quick_Set_Color_Map_1;

        
--------------------------------------------------------------------------------
--        Auxiliary things                                                    --
--------------------------------------------------------------------------------

    -- This is a mask function for Shade_Regions (aka plshades) et al that always
    -- returns 1 so that all points are plotted. Can be used as a template 
    -- for other user-written mask functions. This behave the same as 
    -- when passing null for the second argument in Shade_Regions.
    function Mask_Function_No_Mask(x, y : Long_Float) return Integer is
    begin
        return 1;
    end Mask_Function_No_Mask;


    -- Given an array to hold contour levels and function minimum and maximum,
    -- fill it and return. Useful for contour and shade plots.
    procedure Calculate_Contour_Levels
       (Contour_Levels : in out Real_Vector;
        z_Min, z_Max : Long_Float) is
        
        step : Long_Float;
        ii : Integer;
    begin
        step := (z_Max - z_Min) / Long_Float(Contour_Levels'Last - Contour_Levels'First);
        for i in Contour_Levels'range loop
            ii := i - Contour_Levels'First; -- reference back to 0.
            Contour_Levels(i) := z_Min + step * Long_Float(ii);
        end loop;
    end Calculate_Contour_Levels;


--------------------------------------------------------------------------------
--        Re-define PLplot procedures using Ada style.                        --
--------------------------------------------------------------------------------

-- These correspond to the section in plot.h called "Function Prototypes".

    -- set the format of the contour labels
    -- pl_setcontlabelformat
    procedure Set_Contour_Label_Format
       (Limit_Exponent     : Integer := 4;
        Significant_Digits : Integer := 2) is
    begin
        pl_setcontlabelformat(Limit_Exponent, Significant_Digits);
    end Set_Contour_Label_Format;


    -- set offset and spacing of contour labels
    -- pl_setcontlabelparam
    procedure Set_Contour_Label_Parameters
       (Label_Offset       : Long_Float := 0.006; -- Units are ???
        Label_Font_Height  : Long_Float := 0.3;   -- Units are ???
        Label_Spacing      : Long_Float := 0.1;   -- Units are???
        Labels_Active      : Boolean := False) is
    
        active : Integer;
    
    begin
        if Labels_Active then
            active := 1;
        else
            active := 0;
        end if;
        pl_setcontlabelparam(Label_Offset, Label_Font_Height, Label_Spacing, active);
    end Set_Contour_Label_Parameters;


    -- Advance to subpage "page", or to the next one if "page" = 0.
    -- pladv
    procedure Advance_To_Subpage(Page : Natural) is
    begin
        pladv(Page);
    end Advance_To_Subpage;


    -- fix this
    -- simple arrow plotter.
    -- plarrows
    procedure Arrow_Plotter is
    begin
        Put_Line("Not implemented due to lack of documentation.");
    end Arrow_Plotter;


    -- fix this See comment for plcont.
    -- Draw a 2D vector plot.
    -- plvect
    procedure Vector_Plot
       (u, v                             : Real_Matrix;
        Scale                            : Long_Float;
        Transformation_Procedure_Pointer : Transformation_Procedure_Pointer_Type;
        Transformation_Data              : Transformation_Data_Type) is

        Transformation_Data_Address : PLpointer;
    begin
        Transformation_Data_Address := Transformation_Data'Address;
        plvect(Matrix_To_Pointers(u), Matrix_To_Pointers(v), u'Length(1), u'Length(2), Scale, Transformation_Procedure_Pointer, Transformation_Data_Address);
    end Vector_Plot;
       

    -- Set the style for the arrow used by plvect to plot vectors.
    -- plsvect
    procedure Set_Arrow_Style_For_Vector_Plots
       (X_Vertices, Y_Vertices : Real_Vector; -- Should be range -0.5..0.5
        Fill_Arrow             : Boolean) is
        
        fill : PLBOOL;
        
    begin
        if Fill_Arrow then
            fill := PLtrue;
        else
            fill := PLfalse;
        end if;
        plsvect(X_Vertices, Y_Vertices, X_Vertices'length, fill);
    end Set_Arrow_Style_For_Vector_Plots;
        
        
    -- This functions similarly to plbox() except that the origin of the axes
    -- is placed at the user-specified point (x0, y0).
    -- plaxes
    procedure Box_Around_Viewport_With_Origin
       (X_Origin, Y_Origin       : Long_Float;
        X_Option_String          : String;
        X_Major_Tick_Interval    : Long_Float;
        X_Number_Of_Subintervals : Natural;
        Y_Option_String          : String;
        Y_Major_Tick_Interval    : Long_Float;
        Y_Number_Of_Subintervals : Natural) is
    begin
        plaxes
           (X_Origin, Y_Origin,
            To_C(X_Option_String, True), X_Major_Tick_Interval, X_Number_Of_Subintervals,
            To_C(Y_Option_String, True), Y_Major_Tick_Interval, Y_Number_Of_Subintervals);
    end Box_Around_Viewport_With_Origin;


    -- Plot a histogram using x to store data values and y to store frequencies
    -- plbin
    procedure Histogram_Binned
       (Bin_Values     : Real_Vector; -- "x"
        Bin_Counts     : Real_Vector; -- "y"
        Options        : Integer) is -- Options are not defined in plplot.h.
    begin
        plbin(Bin_Values'length, Bin_Values, Bin_Counts, Options);
    end Histogram_Binned;
        

    -- Start new page. Should only be used with pleop().
    -- plbop
    procedure Begin_New_Page is
    begin
        plbop;
    end Begin_New_Page;


    -- This draws a box around the current viewport.
    -- plbox
    procedure Box_Around_Viewport
       (X_Option_String          : String;
        X_Major_Tick_Interval    : Long_Float;
        X_Number_Of_Subintervals : Natural := 0;
        Y_Option_String          : String;
        Y_Major_Tick_Interval    : Long_Float;
        Y_Number_Of_Subintervals : Natural) is
    begin
        plbox
           (To_C(X_Option_String, True), X_Major_Tick_Interval, X_Number_Of_Subintervals,
            To_C(Y_Option_String, True), Y_Major_Tick_Interval, Y_Number_Of_Subintervals);
    end Box_Around_Viewport;


    -- This is the 3-d analogue of plbox().
    -- plbox3
    procedure Box_Around_Viewport_3D
       (X_Option_String          : String;
        X_Label                  : String := To_String(Default_Label_String);
        X_Major_Tick_Interval    : Long_Float := 0.0;
        X_Number_Of_Subintervals : Natural := 0;

        Y_Option_String          : String;
        Y_Label                  : String := To_String(Default_Label_String);
        Y_Major_Tick_Interval    : Long_Float := 0.0;
        Y_Number_Of_Subintervals : Natural := 0;

        Z_Option_String          : String;
        Z_Label                  : String := To_String(Default_Label_String);
        Z_Major_Tick_Interval    : Long_Float := 0.0;
        Z_Number_Of_Subintervals : Natural := 0) is
    begin
        plbox3
           (To_C(X_Option_String, True), To_C(X_Label, True), X_Major_Tick_Interval, X_Number_Of_Subintervals,
            To_C(Y_Option_String, True), To_C(Y_Label, True), Y_Major_Tick_Interval, Y_Number_Of_Subintervals,
            To_C(Z_Option_String, True), To_C(Z_Label, True), Z_Major_Tick_Interval, Z_Number_Of_Subintervals);
    end Box_Around_Viewport_3D;
        

    -- Calculate world coordinates and subpage from relative device coordinates.
    -- plcalc_world
    procedure World_From_Relative_Coordinates
       (x_Relative, y_Relative : Long_Float_0_1_Type;
        x_World,    y_World    : out Long_Float;
        Last_Window_Index      : out Integer) is
    begin
        plcalc_world(x_Relative, y_Relative, x_World, y_World, Last_Window_Index);
    end World_From_Relative_Coordinates;


    -- Clear current subpage.
    -- plclear
    procedure Clear_Current_Subpage is
    begin
        plclear;
    end Clear_Current_Subpage;


    -- Set color, map 0. Argument is integer between 0 and 15.
    -- plcol0
    procedure Set_Pen_Color(A_Color : Plot_Color_Type) is
    begin
        plcol0(A_Color);
    end Set_Pen_Color;


    -- Set color, map 1. Argument is a float between 0. and 1.
    -- plcol1
    procedure Set_Color_Map_1(Color : Long_Float_0_1_Type) is
    begin
        plcol1(Color);
    end Set_Color_Map_1;


    -- Draws a contour plot from data in f(nx,ny). Is just a front-end to
    -- plfcont, with a particular choice for f2eval and f2eval_data.

-- fix this
---------- Ada binding note of December 7, 2006 (JB) ------------
--  This is mostly "fixed," but calls to pltr1 (and presumably pltr2) fail.
--  The subroutine pltr1 (in file plcont.c) seems to be locked in to the 
--  zero-based C array indexing style. For example, this line checks for array 
--  bounds and exits if they are not zero-based:
--    if (x < 0 || x > nx - 1 || y < 0 || y > ny - 1) {
--  	plexit("pltr1: Invalid coordinates");
--  Setting the Ada arrays in Transformation_Data_Type to begin indexing at 0 
--  prevents the above abort to happen (but forces zero-based indexing in your
--  Ada arrays).
--  HOWEVER, this then causes the following error:
--    *** PLPLOT ERROR ***
--    plfcont: indices must satisfy  1 <= kx <= lx <= nx, aborting operation
--  For now, only calls to pltr0 (no mapping) do not cause problems.

    --  plcont
    procedure Contour_Plot
       (z                                : Real_Matrix;
        x_Min_Index, x_Max_Index         : Integer;
        y_Min_Index, y_Max_Index         : Integer;
        Contour_Levels                   : Real_Vector;
        Transformation_Procedure_Pointer : Transformation_Procedure_Pointer_Type;
        Transformation_Data              : Transformation_Data_Type) is
        
        -- This is a good place to change from the convenient form of passing 
        -- the transformation data (a record) to the form required by the PLplot 
        -- API (a pointer).
        Transformation_Data_Address : PLpointer;
    begin
        Transformation_Data_Address := Transformation_Data'Address;
        plcont(Matrix_To_Pointers(z), z'Length(1), z'Length(2), 
            x_Min_Index, x_Max_Index, y_Min_Index, y_Max_Index, Contour_Levels, 
            Contour_Levels'Length, Transformation_Procedure_Pointer, 
            Transformation_Data_Address);
    end Contour_Plot;


    -- The procedure plfcont is not documented and is not part of the API. 
    -- However, it is a very useful capability to have available.
    -- I have tried to implement it as I think was intended but this may be incorrect.
    -- It appears as though the intent is to pass the arbitrarily organized 
    -- data (pointed to by Irregular_Data_Pointer) as a (single) pointer to a 
    -- 2D C-style array. Thus, for examaple, it is not possible to pass the data  
    -- as triples.
    
    -- Draws a contour plot using the function evaluator f2eval and data stored
    -- by way of the f2eval_data pointer. This allows arbitrary organizations
    -- of 2d array data to be used.

    -- plfcont
    procedure Contour_Plot_Irregular_Data
       (Function_Evaluator_Pointer       : Function_Evaluator_Pointer_Type;
        Irregular_Data                   : Real_Matrix;
        x_Min_Index, x_Max_Index         : Integer;
        y_Min_Index, y_Max_Index         : Integer;
        Contour_Levels                   : Real_Vector;
        Transformation_Procedure_Pointer : Transformation_Procedure_Pointer_Type;
        Transformation_Data              : Transformation_Data_Type) is

        -- This is a good place to change from the convenient form of passing 
        -- the transformation data (a record) to the form required by the PLplot 
        -- API (a pointer); same for the irregularly spaced data matrix.
        Transformation_Data_Address : PLpointer;
        Irregular_Data_Address      : PLpointer;
    begin
        Transformation_Data_Address := Transformation_Data'Address;
        Irregular_Data_Address      := Irregular_Data'Address;

        plfcont(Function_Evaluator_Pointer, Irregular_Data_Address, 
            Irregular_Data'Length(1), Irregular_Data'Length(2), 
            x_Min_Index, x_Max_Index, y_Min_Index, y_Max_Index, 
            Contour_Levels, Contour_Levels'Length, 
            Transformation_Procedure_Pointer, Transformation_Data_Address);
    end Contour_Plot_Irregular_Data;


    -- Copies state parameters from the reference stream to the current stream.
    -- plcpstrm
    procedure Copy_State_Parameters
       (Stream_ID                      : Integer;
        Do_Not_Copy_Device_Coordinates : Boolean) is
    
        PL_Do_Not_Copy_Device_Coordinates : PLBOOL;
        
    begin
        if Do_Not_Copy_Device_Coordinates then
            PL_Do_Not_Copy_Device_Coordinates := PLtrue;
        else
            PL_Do_Not_Copy_Device_Coordinates := PLfalse;
        end if;
        plcpstrm(Stream_ID, PL_Do_Not_Copy_Device_Coordinates);
    end Copy_State_Parameters;


    -- fix this
    -- Converts input values from relative device coordinates to relative plot
    -- coordinates.
    -- pldid2pc
    procedure pldid2pc_Placeholder is
    begin
        Put_Line("Not implemented due to lack of documentation.");
    end pldid2pc_Placeholder;


    -- fix this
    -- Converts input values from relative plot coordinates to relative
    -- device coordinates.
    -- pldip2dc
    procedure pldip2dc_Placeholder is
    begin
        Put_Line("Not implemented due to lack of documentation.");
    end pldip2dc_Placeholder;


    -- End a plotting session for all open streams.
    -- plend
    procedure End_PLplot is
    begin
        plend;
    end End_PLplot;


    -- End a plotting session for the current stream only.
    -- plend1
    procedure End_PLplot_Current_Stream is
    begin
        plend1;
    end End_PLplot_Current_Stream;


    -- Simple interface for defining viewport and window.
    -- plenv
    procedure Set_Environment
       (x_Min, x_Max, y_Min, y_Max : Long_Float;
        Justification              : Justification_Type;
        Axis_Style                 : Axis_Style_Type) is
    begin
        plenv(x_Min, x_Max, y_Min, y_Max, Justification, Axis_Style);
    end Set_Environment;


    -- similar to plenv() above, but in multiplot mode does not advance the subpage,
    -- instead the current subpage is cleared
    -- plenv0
    procedure Set_Environment_Clear_Subpage
       (x_Min, x_Max, y_Min, y_Max : Long_Float;
        Justification              : Justification_Type;
        Axis_Style                 : Axis_Style_Type) is
    begin
        plenv0(x_Min, x_Max, y_Min, y_Max, Justification, Axis_Style);
    end Set_Environment_Clear_Subpage;


    -- End current page. Should only be used with plbop().
    -- pleop
    procedure Eject_Current_Page is
    begin
        pleop;
    end Eject_Current_Page;


    -- Plot horizontal error bars (xmin(i),y(i)) to (xmax(i),y(i))
    -- plerrx
    procedure Draw_Error_Bars_X(x_Min, x_Max, y : Real_Vector) is
    begin
        plerrx(x_Min'Length, x_Min, x_Max, y);
    end Draw_Error_Bars_X;


    -- Plot vertical error bars (x,ymin(i)) to (x(i),ymax(i))
    -- plerry
    procedure Draw_Error_Bars_Y(x, y_Min, y_Max : Real_Vector) is
    begin
        plerry(y_Min'Length, x, y_Min, y_Max);
    end Draw_Error_Bars_Y;


    -- Advance to the next family file on the next new page
    -- plfamadv
    procedure Next_Family_File_On_New_Page is
    begin
        plfamadv;
    end Next_Family_File_On_New_Page;


    -- Pattern fills the polygon bounded by the input points.
    -- plfill
    procedure Fill_Polygon(x, y : Real_Vector) is
    begin
        plfill(x'Length, x, y);
    end Fill_Polygon;


    -- Pattern fills the 3d polygon bounded by the input points.
    -- plfill3
    procedure Fill_Polygon_3D (x, y, z : Real_Vector) is
    begin
        plfill3(x'Length, x, y, z);
    end Fill_Polygon_3D;


    -- Flushes the output stream. Use sparingly, if at all.
    -- plflush
    procedure Flush_Output_Stream is
    begin
        plflush;
    end Flush_Output_Stream;


    -- Sets the global font flag to 'ifont'.
    -- plfont
    procedure Set_Font_Style(Font_Style : Font_Style_Type) is
    begin
        plfont(Font_Style);
    end Set_Font_Style;


    -- Load specified font set.
    -- plfontld
    procedure Set_Characer_Set(Character_Set : Character_Set_Type) is
    begin
        plfontld(Character_Set);
    end Set_Characer_Set;


    -- Get character default height and current (scaled) height
    -- plgchr
    procedure Get_Character_Height(Default_Height, Current_Height : out Long_Float) is
    begin
        plgchr(Default_Height, Current_Height);
    end Get_Character_Height;


    -- Returns 8 bit RGB values for given color from color map 0
    -- plgcol0
    procedure Get_Color_RGB
       (Color_Index      : Integer;
        Red_Component, Green_Component, Blue_Component : out Integer) is
    begin
        plgcol0(Color_Index, Red_Component, Green_Component, Blue_Component);
    end Get_Color_RGB;


    -- Returns the background color by 8 bit RGB value
    -- plgcolbg
    procedure Get_Background_Color_RGB
       (Red_Component, Green_Component, Blue_Component : out Integer) is
    begin
        plgcolbg(Red_Component, Green_Component, Blue_Component);
    end Get_Background_Color_RGB;


    -- Returns the current compression setting
    -- plgcompression
    procedure Get_Compression_Level(Compression_Level : out Integer) is
    begin
        plgcompression(Compression_Level);
    end Get_Compression_Level;


    -- Get the current device (keyword) name
    -- plgdev
    procedure Get_Device_Name(Device_Name : out String) is
        PL_Device_Name : char_array(0..79);
    begin
        plgdev(PL_Device_Name);
        Device_Name := To_Ada(PL_Device_Name, True);
    end Get_Device_Name;


    -- Retrieve current window into device space
    -- plgdidev
    procedure Get_Device_Window_Parameters
       (Relative_Margin_Width : out Long_Float;
        Aspect_Ratio          : out Long_Float;
        x_Justification       : out Long_Float;
        y_Justification       : out Long_Float) is
    begin
        plgdidev(Relative_Margin_Width, Aspect_Ratio, x_Justification, y_Justification);
    end Get_Device_Window_Parameters;


    -- Get plot orientation
    -- plgdiori
    procedure Get_Plot_Orientation(Rotation : out Long_Float) is
    begin
        plgdiori(Rotation);
    end Get_Plot_Orientation;


    -- Retrieve current window into plot space
    -- plgdiplt
    procedure Get_Device_Window_Extrema
       (x_Min : out Long_Float;
        y_Min : out Long_Float;
        x_Max : out Long_Float;
        y_Max : out Long_Float) is
    begin
        plgdiplt(x_Min, y_Min, x_Max, y_Max);
    end Get_Device_Window_Extrema;


    -- Get FCI (font characterization integer)
    -- plgfci
    procedure Get_Font_Characterization_Integer(Font_Characterization_Integer : out PLUNICODE) is
    begin
        plgfci(Font_Characterization_Integer);
    end Get_Font_Characterization_Integer;


    -- Get family file parameters
    -- plgfam
    procedure Get_File_Family_Parameters
       (Family_Enabled : out Boolean;
        Family_File_Number : out Integer;
        Maximum_File_Size : out Integer) is
        
        PL_Family_Enabled : PLBOOL;
        
    begin
        plgfam(PL_Family_Enabled, Family_File_Number, Maximum_File_Size);
        if PL_Family_Enabled = 0 then
            Family_Enabled := False;
        else
            Family_Enabled := True;
        end if;
    end Get_File_Family_Parameters;


    -- Get the (current) output file name. Must be preallocated to >80 bytes
    -- plgfnam
    procedure Get_Output_File_Name(Output_File_Name : out String) is
        PL_Output_File_Name : char_array(0..79);
    begin
        plgfnam(PL_Output_File_Name);
        Output_File_Name := To_Ada(PL_Output_File_Name, True);
    end Get_Output_File_Name;


    -- Get the (current) run level.
    -- plglevel
    procedure Get_Run_Level(Run_Level : out Integer) is
    begin
        plglevel(Run_Level);
    end Get_Run_Level;


    -- Get output device parameters.
    -- plgpage
    procedure Get_Page_Parameters
       (x_Pixels, y_Pixels           : out Long_Float;
        x_Page_Length, y_Page_Length : out Integer;
        x_Page_Offset, y_Page_Offset : out Integer) is
    begin
        plgpage(x_Pixels, y_Pixels, x_Page_Length, y_Page_Length, x_Page_Offset, y_Page_Offset);
    end Get_Page_Parameters;


    -- Switches to graphics screen.
    -- plgra
    procedure Use_Graphics_Mode is
    begin
        plgra;
    end Use_Graphics_Mode;


    -- Grid irregularly sampled data.
    -- plgriddata
    procedure Grid_Data
       (x, y, z                : Real_Vector; -- ungridded x- and y-points; z is height
        x_Grid, y_Grid         : Real_Vector;
        z_Gridded              : in out Real_Matrix;
        Gridding_Algorithm     : Gridding_Algorithm_Type;
        Griding_Algorithm_Data : Long_Float) is
    begin
        -- Note that since z_Gridded is converted to and passed as an array of 
        -- pointers, this is effectively the same as pass-by-reference so 
        -- "in out" is not required for the formal argument zg in plgriddata.
        plgriddata(x, y, z, x'Length, x_Grid, x_Grid'Length, y_Grid, y_Grid'Length, 
            Matrix_To_Pointers(z_Gridded), Gridding_Algorithm, Griding_Algorithm_Data);            
    end Grid_Data;


    -- Get subpage boundaries in absolute coordinates
    -- Results are millimeters from the lower left corner.
    -- plgspa
    procedure Get_Subpage_Boundaries
       (Left_Edge, Right_Edge, Bottom_Edge, Top_Edge : out Long_Float) is
    begin
        plgspa(Left_Edge, Right_Edge, Bottom_Edge, Top_Edge);
    end Get_Subpage_Boundaries;


    -- Get current stream number.
    -- plgstrm
    procedure Get_Stream_Number(Stream_Number : out Integer) is
    begin
        plgstrm(Stream_Number);
    end Get_Stream_Number;


    -- Get the current library version number
    -- plgver
    procedure Get_Version_Number(Version_Number : out String) is
        PL_Version_Number : char_array(0..79);
    begin
        plgver(PL_Version_Number);
        Version_Number := To_Ada(PL_Version_Number, True);
    end Get_Version_Number;


    -- Function version of the procedure Get_Version_Number; not part of the PLplot API.
    -- plgver
    function Get_Version_Number return String is
        PL_Version_Number : char_array(0..79);
    begin
        plgver(PL_Version_Number);
        return To_Ada(PL_Version_Number, True);
    end Get_Version_Number;


    -- Get viewport boundaries in normalized device coordinates
    -- plgvpd
    procedure Get_Viewport_Normalized(x_Min, x_Max, y_Min, y_Max : out Long_Float) is
    begin
        plgvpd(x_Min, x_Max, y_Min, y_Max);
    end Get_Viewport_Normalized;


    -- Get viewport boundaries in world coordinates
    -- plgvpw
    procedure Get_Viewport_World(x_Min, x_Max, y_Min, y_Max : out Long_Float) is
    begin
        plgvpw(x_Min, x_Max, y_Min, y_Max);
    end Get_Viewport_World;


    -- Get x axis labeling parameters
    -- plgxax
    procedure Get_X_Label_Parameters(Max_Digits, Actual_Digits : out Integer) is
    begin
        plgxax(Max_Digits, Actual_Digits);
    end Get_X_Label_Parameters;


    -- Get y axis labeling parameters
    -- plgyax
    procedure Get_Y_Label_Parameters(Max_Digits, Actual_Digits : out Integer) is
    begin
        plgyax(Max_Digits, Actual_Digits);
    end Get_Y_Label_Parameters;


    -- Get z axis labeling parameters
    -- plgzax
    procedure Get_Z_Label_Parameters(Max_Digits, Actual_Digits : out Integer) is
    begin
        plgzax(Max_Digits, Actual_Digits);
    end Get_Z_Label_Parameters;


    -- Draws a histogram of n values of a variable in array data[0..n-1]
    -- plhist
    procedure Histogram_Unbinned
       (Data           : Real_Vector;
        Data_Min       : Long_Float; -- left  edge of left-most bin
        Data_Max       : Long_Float; -- right edge of right-most bin
        Number_Of_Bins : Positive; -- equal-sized, between Data_Min and Data_Max
        Options : Integer) is -- Options are not defined in plplot.h.
    begin
        Set_Pen_Color(White);
        plhist
           (Data'length, Data, Data_Min, Data_Max, Number_Of_Bins, Options);
        Set_Pen_Color(White);
    end Histogram_Unbinned;


    -- Set current color (map 0) by hue, lightness, and saturation.
    -- plhls
    procedure Set_Color_HLS
       (Hue_Component        : Long_Float; -- Not documented; presumably 0.0..360.0.
        Lightness_Component  : Long_Float_0_1_Type;
        Saturation_Component : Long_Float_0_1_Type) is
    begin
        plhls(Hue_Component, Lightness_Component, Saturation_Component);
    end Set_Color_HLS;


    -- Functions for converting between HLS and RGB color space
    -- plhlsrgb
    procedure HLS_To_RGB
       (Hue_Component        : Long_Float;
        Lightness_Component  : Long_Float_0_1_Type;
        Saturation_Component : Long_Float_0_1_Type;
        Red_Component        : out Long_Float_0_1_Type;
        Green_Component      : out Long_Float_0_1_Type;
        Blue_Component       : out Long_Float_0_1_Type) is
    begin
        plhlsrgb(Hue_Component, Lightness_Component, Saturation_Component, Red_Component, Green_Component, Blue_Component);
    end HLS_To_RGB;


    -- Initialization. Must be called before starting plot.
    -- Certain other procedures, if used, must be called first.
    -- Initializes PLplot, using preset or default options
    -- plinit
    procedure Initialize_PLplot is
    begin
        plinit;
    end Initialize_PLplot;


    -- Draws a line segment from (x1, y1) to (x2, y2).
    -- pljoin
    procedure Draw_Line(x1, y1, x2, y2 : Long_Float) is
    begin
        pljoin(x1, y1, x2, y2);
    end Draw_Line;


    -- Simple routine for labelling graphs.
    -- pllab
    procedure Write_Labels(X_Label, Y_Label, Title_Label : String := To_String(Default_Label_String)) is
    begin
        pllab(To_C(X_Label, True), To_C(Y_Label, True), To_C(Title_Label, True));
    end Write_Labels;


    -- Sets position of the light source
    -- pllightsource
    procedure Set_Light_Source
       (x : Long_Float := 1.0;
        y: Long_Float := 1.0;
        z: Long_Float := 1.0) is
    begin
        pllightsource(x, y, z);
    end Set_Light_Source;


    -- Draws line segments connecting a series of points.
    -- plline
    procedure Draw_Curve(x, y : Real_Vector) is
    begin
        plline(x'Length, x, y);
    end Draw_Curve;


    -- Draws a line in 3 space.
    -- plline3
    procedure Draw_Curve_3D(x, y, z : Real_Vector) is
    begin
        plline3(x'Length, x, y, z);
    end Draw_Curve_3D;


    -- Set line style.
    -- pllsty
    procedure Select_Line_Style(Line_Style : Line_Style_Type) is
    begin
        pllsty(Line_Style);
    end Select_Line_Style;


    -- plot continental outline in world coordinates
    -- plmap
    procedure Draw_Map
       (Map_Form_Function_Pointer            : Map_Form_Function_Pointer_Type;
        Map_Kind                             : Map_Type;
        Minimum_Longitude, Maximum_Longitude : Long_Float;
        Minimum_Latitude,  Maximum_Latitude  : Long_Float) is
        
        Map_Kind_String : Unbounded_String;
    begin
        -- Convert Map_Kind into a string; later, a C string.
        case Map_Kind is
            when Continents =>
                begin
                    Map_Kind_String := TUB("globe");
                end;
            when USA_and_States =>
                begin
                    Map_Kind_String := TUB("usa");
                end;
            when Continents_and_Countries =>
                begin
                    Map_Kind_String := TUB("cglobe");
                end;
            when USA_States_and_Continents =>
                begin
                    Map_Kind_String := TUB("usaglobe");
                end;
        end case;
        
        plmap(Map_Form_Function_Pointer, To_C(To_String(Map_Kind_String), True), 
            Minimum_Longitude, Maximum_Longitude, Minimum_Latitude, Maximum_Latitude);
    end Draw_Map;


    -- Plot the latitudes and longitudes on the background.
    -- plmeridians
    procedure Draw_Latitude_Longitude
       (Map_Form_Function_Pointer            : Map_Form_Function_Pointer_Type;
        Delta_Longitude, Delta_Latitude      : Long_Float;
        Minimum_Longitude, Maximum_Longitude : Long_Float;
        Minimum_Latitude,  Maximum_Latitude  : Long_Float) is
    begin
        plmeridians(Map_Form_Function_Pointer, Delta_Longitude, Delta_Latitude, 
            Minimum_Longitude, Maximum_Longitude, Minimum_Latitude, Maximum_Latitude);
    end Draw_Latitude_Longitude;


    -- Plots a mesh representation of the function z[x][y].
    -- plmesh
    procedure Mesh_3D
       (x, y    : Real_Vector; -- surface definition points
        z       : Real_Matrix; -- height of surface at definition points
        Options : Integer) is
    begin        
        plmesh(x, y, Matrix_To_Pointers(z), x'Length, y'Length, Options);
    end Mesh_3D;


    -- Plots a mesh representation of the function z[x][y] with contour
    -- plmeshc
    procedure Mesh_3D_Base_Contour -- prototype
       (x, y           : Real_Vector; -- surface definition points
        z              : Real_Matrix; -- height of surface at definition points
        Options        : Integer;
        Contour_Levels : Real_Vector) is -- levels at which to draw contours
    begin        
        plmeshc(x, y, Matrix_To_Pointers(z), x'Length, y'Length, Options, Contour_Levels, Contour_Levels'Length);
    end Mesh_3D_Base_Contour;


    -- Creates a new stream and makes it the default.
    -- plmkstrm
    procedure Make_Stream(New_Stream_Number : out Integer) is
    begin
        plmkstrm(New_Stream_Number);
    end Make_Stream;


    -- Prints out "text" at specified position relative to viewport
    -- plmtex
    procedure Write_Text_Viewport
       (Side : String;
        Position_From_Edge : Long_Float;
        Position_Along_Edge : Long_Float;
        Justification : Long_Float;
        The_Text : String) is
    begin
        plmtex(To_C(Side, True), Position_From_Edge, Position_Along_Edge, Justification, To_C(The_Text, True));
    end Write_Text_Viewport;


    -- Plots a 3-d representation of the function z[x][y].
    -- plot3d
    procedure Plot_3D
       (x, y    : Real_Vector; -- surface definition points
        z       : Real_Matrix; -- height of surface at definition points
        Options : Integer;
        Sides   : Boolean) is -- draw sides?    

        PLsides : PLBOOL;
    
    begin
        if Sides then
            PLsides := PLtrue;
        else
            PLsides := PLfalse;
        end if;
        plot3d(x, y, Matrix_To_Pointers(z), x'Length, y'Length, Options, PLsides);
    end Plot_3D;


    -- Plots a 3-d representation of the function z[x][y] with contour.
    -- plot3dc
    procedure Plot_3D_Base_Contour
       (x, y           : Real_Vector; -- surface definition points
        z              : Real_Matrix; -- height of surface at definition points
        Options        : Integer;
        Contour_Levels : Real_Vector) is -- levels at which to draw contours
    begin        
        plot3dc(x, y, Matrix_To_Pointers(z), x'Length, y'Length, Options, Contour_Levels, Contour_Levels'Length);
    end Plot_3D_Base_Contour;


    -- fix this; update variable names when documentation becomes available.
    -- Plots a 3-d representation of the function z[x][y] with contour and
    -- y index limits.
    -- plot3dcl
    procedure Plot_3D_Base_Contour_Limits -- Lacks documentation in Chapter 17 of Ref. Man.
       (x, y                 : Real_Vector; -- surface definition points
        z                    : Real_Matrix; -- height of surface at definition points
        Options              : Integer;
        Contour_Levels       : Real_Vector;
        ixstart, ixn         : Integer;
        indexymin, indexymax : Integer_Array_1D) is -- levels at which to draw contours
    begin        
        plot3dcl(x, y, Matrix_To_Pointers(z), x'Length, y'Length, Options, Contour_Levels, Contour_Levels'Length, ixstart, ixn, indexymin, indexymax);
    end Plot_3D_Base_Contour_Limits;
 

    -- Set fill pattern directly.
    -- plpat
    procedure Set_Fill_Pattern
       (Inclinations : Integer_Array_1D;
        Spacings     : Integer_Array_1D) is
    begin
        plpat(Inclinations'Length, Inclinations, Spacings);
    end Set_Fill_Pattern;


    -- Plots array y against x for n points using ASCII code "code".
    -- plpoin
    procedure Draw_Points
       (x, y             : Real_Vector;
        Symbol_As_Number : Integer) is
    begin
        plpoin(x'Length, x, y, Symbol_As_Number);
    end Draw_Points;
    


    -- Draws a series of points in 3 space.
    -- plpoin3
    procedure Draw_Points_3D
       (x, y, z          : Real_Vector;
        Symbol_As_Number : Integer) is
    begin
        plpoin3(x'Length, x, y, z, Symbol_As_Number);
    end Draw_Points_3D;


    -- Draws a polygon in 3 space.
    -- plpoly3
    procedure Draw_Polygon_3D
       (x, y, z : Real_Vector;
        Draw_Segments : Boolean_Array_1D;
        Draw_Counterclockwise : Boolean) is
    
        PL_Draw_Segments : PL_Bool_Array(Draw_Segments'range);
        PL_Draw_Counterclockwise : PLBOOL;
    
    begin
        if Draw_Counterclockwise then
            PL_Draw_Counterclockwise := PLTrue;
        else
            PL_Draw_Counterclockwise := PLfalse;
        end if;
        
        for Index in Draw_Segments'range loop
            if Draw_Segments(Index) then
                PL_Draw_Segments(Index) := PLtrue;
            else
                PL_Draw_Segments(Index) := PLfalse;
            end if;
        end loop;
        plpoly3(x'length, x, y, z, PL_Draw_Segments, PL_Draw_Counterclockwise);
    end Draw_Polygon_3D;


    -- Set the floating point precision (in number of places) in numeric labels.
    -- plprec
    procedure Set_Numeric_Label_Precision
       (Set_Automatically : Boolean := False;
        Number_Digits_After_Decimal_Point : Integer := 2) is
    begin
        if Set_Automatically then
            plprec(0, Number_Digits_After_Decimal_Point);
        else
            plprec(1, Number_Digits_After_Decimal_Point);
        end if;
    end Set_Numeric_Label_Precision;


    -- Set fill pattern, using one of the predefined patterns.
    -- plpsty
    procedure Select_Fill_Pattern(Fill_Pattern : Fill_Pattern_Type) is
    begin
        plpsty(Fill_Pattern);
    end Select_Fill_Pattern;


    -- Prints out "text" at world cooordinate (x,y).
    -- plptex
    procedure Write_Text_World
       (x, y             : Long_Float;
        Delta_X, Delta_Y : Long_Float;
        Justification    : Long_Float;
        The_Text         : String) is
    begin
        plptex(x, y, Delta_X, Delta_Y, Justification, To_C(The_Text, True));
    end Write_Text_World;


    -- Replays contents of plot buffer to current device/file.
    -- plreplot
    procedure Replot is
    begin
        plreplot;
    end Replot;


    -- Set line color by red, green, blue from  0. to 1.
    -- plrgb
    procedure Set_Line_Color_RGB_0_1(Red_Component, Blue_Component, Green_Component : Long_Float_0_1_Type) is
    begin
        plrgb(Red_Component, Blue_Component, Green_Component);
    end Set_Line_Color_RGB_0_1;


    -- Set line color by 8 bit RGB values.
    -- plrgb1
    procedure Set_Line_Color_RGB_0_255(Red_Component, Blue_Component, Green_Component : Integer) is
    begin
        plrgb1(Red_Component, Blue_Component, Green_Component);
    end Set_Line_Color_RGB_0_255;


    -- Functions for converting between HLS and RGB color space
    -- plrgbhls
    procedure RGB_To_HLS
       (Red_Component        : Long_Float_0_1_Type;
        Green_Component      : Long_Float_0_1_Type;
        Blue_Component       : Long_Float_0_1_Type;
        Hue_Component        : out Long_Float;
        Lightness_Component  : out Long_Float_0_1_Type;
        Saturation_Component : out Long_Float_0_1_Type) is
    begin
        plrgbhls(Red_Component, Green_Component, Blue_Component, Hue_Component, Lightness_Component, Saturation_Component);
    end RGB_To_HLS;


    -- Set character height.
    -- plschr
    procedure Set_Character_Height(Default_Height, Scale_Factor : Long_Float) is
    begin
        plschr(Default_Height, Scale_Factor);
    end Set_Character_Height;


    -- The PLplot docs say that the arguments to this procedure are arrays of 8-bit numbers
    -- but plplot.h says that they are arrays of 32-bit integers.
    -- Set color map 0 colors by 8 bit RGB values
    -- plscmap0
    procedure Set_Color_Map_0(Red_Components, Green_Components, Blue_Components : Integer_Array_1D) is
    begin
        plscmap0(Red_Components, Green_Components, Blue_Components, Red_Components'Length);
    end Set_Color_Map_0;


    -- Set number of colors in cmap 0
    -- plscmap0n
    procedure Set_Number_Of_Colors_Map_0(Number_Of_Colors : Integer) is
    begin
        plscmap0n(Number_Of_Colors);
    end Set_Number_Of_Colors_Map_0;


    -- Set color map 1 colors by 8 bit RGB values
    -- plscmap1
    procedure Set_Color_Map_1_RGB
       (Red_Component, Green_Component, Blue_Component : Integer_0_255_Array) is
       
       PL_Red_Component, PL_Green_Component, PL_Blue_Component : Integer_Array_1D(Red_Component'range);
    begin
        -- Copy constrained color integers to unconstrained integers.
        for i in Red_Component'range loop
            PL_Red_Component(i)   := Red_Component(i);
            PL_Green_Component(i) := Green_Component(i);
            PL_Blue_Component(i)  := Blue_Component(i);
        end loop;
        plscmap1(PL_Red_Component, PL_Green_Component, PL_Blue_Component, PL_Red_Component'Length);
    end Set_Color_Map_1_RGB;


    -- Set color map 1 colors using a piece-wise linear relationship between
    -- intensity [0,1] (cmap 1 index) and position in HLS or RGB color space.
    -- plscmap1l
    procedure Set_Color_Map_1_Piecewise
       (Color_Model    : Color_Model_Type;    -- HLS or RGB
        Control_Points : Real_Vector; -- range 0.0 .. 1.0; not checked here
        H_Or_R         : Real_Vector; -- range 0.0 .. 1.0; not checked here
                                              -- Note: Hue is 0.0 .. 360.0.
        L_Or_G         : Real_Vector; -- range 0.0 .. 1.0; not checked here
        S_Or_B         : Real_Vector; -- range 0.0 .. 1.0; not checked here
        Reverse_Hue    : Boolean_Array_1D) is -- False means red<->green<->blue<->red, True reverses
        
        PL_Color_Model : PLBOOL;
        PL_Reverse_Hue : PL_Bool_Array (Reverse_Hue'range);
        
    begin
        if Color_Model = RGB then
            PL_Color_Model := PLtrue;
        else
            PL_Color_Model := PLfalse;
        end if;

        for i in Reverse_Hue'range loop
            if Reverse_Hue(i) then
                PL_Reverse_Hue(i) := PLtrue;
            else
                PL_Reverse_Hue(i) := PLfalse;
            end if;
        end loop;

        plscmap1l(PL_Color_Model, Control_Points'Length, Control_Points, H_Or_R, L_Or_G, S_Or_B, PL_Reverse_Hue);
    end Set_Color_Map_1_Piecewise;


    -- Set number of colors in cmap 1
    -- plscmap1n
    procedure Set_Number_Of_Colors_In_Color_Map_1(Number_Of_Colors : Integer) is
    begin
        plscmap1n(Number_Of_Colors);
    end Set_Number_Of_Colors_In_Color_Map_1;
    

    -- Set a given color from color map 0 by 8 bit RGB value
    -- plscol0
    procedure Set_One_Color_Map_0
       (Plot_Color : Plot_Color_Type;
        Red_Component, Green_Component, Blue_Component : Integer) is
    begin
        plscol0(Plot_Color, Red_Component, Green_Component, Blue_Component);
    end Set_One_Color_Map_0;


    -- Set the background color by 8 bit RGB value
    -- plscolbg
    procedure Set_Background_Color_RGB
       (Red_Component, Green_Component, Blue_Component : Integer) is
    begin
        plscolbg(Red, Green, Blue);
    end Set_Background_Color_RGB;


    -- Used to globally turn color output on/off
    -- plscolor
    procedure Enable_Color_Output(Enable_Color : Boolean) is

        PL_Enable_Color : PLBOOL;
        
    begin
        if Enable_Color then
            PL_Enable_Color := 1;
        else
            PL_Enable_Color := 0;
        end if;
        plscolor(PL_Enable_Color);
    end Enable_Color_Output;


    -- Set the compression level
    -- plscompression
    procedure Set_Compression_Level(Compression_Level : Integer) is
    begin
        plscompression(Compression_Level);
    end Set_Compression_Level;


    -- Set the device (keyword) name
    -- plsdev
    procedure Set_Device_Name(Device_Name : String) is
    begin
        plsdev(To_C(Device_Name, True));
    end Set_Device_Name;


    -- Set window into device space using margin, aspect ratio, and
    -- justification
    -- plsdidev
    procedure Set_Device_Window_Parameters
       (Margin          : Long_Float;
        Aspect_Ratio    : Long_Float;
        x_Justification : Long_Float;
        y_Justification : Long_Float) is
    begin
        plsdidev(Margin, Aspect_Ratio, x_Justification, y_Justification);
    end Set_Device_Window_Parameters;


    -- Set up transformation from metafile coordinates.
    -- plsdimap
    procedure Set_Metafile_Transformation
       (dimxmin : Integer;
        dimxmax : Integer;
        dimymin : Integer;
        dimymax : Integer;
        dimxpmm : Long_Float;
        dimypmm : Long_Float) is
    begin
        plsdimap(dimxmin, dimxmax, dimymin, dimymax, dimxpmm, dimypmm);
    end Set_Metafile_Transformation;
    

    -- Set plot orientation, specifying rotation in units of pi/2.
    -- plsdiori
    procedure Set_Plot_Orientation(Rotation : Long_Float) is
    begin
        plsdiori(Rotation);
    end Set_Plot_Orientation;


    -- Set window into plot space
    -- plsdiplt
    procedure Set_Device_Window_Extrema
       (x_Min : Long_Float := 0.0;
        y_Min : Long_Float := 0.0;
        x_Max : Long_Float := 1.0;
        y_Max : Long_Float := 1.0) is
    begin
        plsdiplt(x_Min, y_Min, x_Max, y_Max);
    end Set_Device_Window_Extrema;
    

    -- Set window into plot space incrementally (zoom)
    -- plsdiplz
    procedure Set_Zoom
       (x_Min_Relative : Long_Float;
        y_Min_Relative : Long_Float;
        x_Max_Relative : Long_Float;
        y_Max_Relative : Long_Float) is
    begin
        plsdiplz(x_Min_Relative, y_Min_Relative, x_Max_Relative, y_Max_Relative);
    end Set_Zoom;


    -- Set the escape character for text strings.
    -- plsesc
    procedure Set_Escape_Character(Escape_Character : Character) is
    
        Escape_Characters : Ada.Strings.Maps.Character_Set;
        
    begin

        Escape_Characters := Ada.Strings.Maps.To_Set("!#$%&*@^~");
        
        if Ada.Strings.Maps.Is_In(Escape_Character, Escape_Characters) then
            plsesc(To_C(Escape_Character));
        else
            Put_Line("Illegal escape character passed to Set_Escape_Character.");
        end if;
    end Set_Escape_Character;


    -- Set family file parameters
    -- plsfam
    procedure Set_File_Family_Parameters
       (Enable_Family : Boolean;
        Family_File_Number : Integer := 1;
        Maximum_File_Size : Integer := 1_000_000) is
    
        PL_Enable_Family : PLBOOL;
        
    begin
        if Enable_Family then
            PL_Enable_Family := 1;
        else
            PL_Enable_Family := 0;
        end if;
        plsfam(PL_Enable_Family, Family_File_Number, Maximum_File_Size);
    end Set_File_Family_Parameters;


    -- Set FCI (font characterization integer)
    -- plsfci;
    procedure Set_Font_Characterization_Integer(Font_Characterization_Integer : PLUNICODE) is
    begin
        plsfci(Font_Characterization_Integer);
    end Set_Font_Characterization_Integer;


    -- Set the output file name.
    -- plsfnam
    procedure Set_Output_File_Name(Output_File_Name : String) is
    begin
        plsfnam(To_C(Output_File_Name, True));
    end Set_Output_File_Name;


    -- Shade region.
    -- plshade
    procedure Shade_Region
       (z                                        : Real_Matrix;
        Mask_Function_Pointer                    : Mask_Function_Pointer_Type;
        x_Min, x_Max, y_Min, y_Max               : Long_Float; -- world mins and maxes
        Shade_Min, Shade_Max                     : Long_Float;
        Select_Color_Map                         : Natural; -- should be 0 or 1
        Color                                    : Long_Float;
        Fill_Pattern_Pen_Width                   : Positive;
        Shade_Min_Pen_Color, Shade_Min_Pen_Width : Natural;
        Shade_Max_Pen_Color, Shade_Max_Pen_Width : Natural;
        Preserve_Rectangles                      : Boolean;
        Transformation_Procedure_Pointer         : Transformation_Procedure_Pointer_Type;
        Transformation_Data                      : Transformation_Data_Type) is

        Transformation_Data_Address : PLpointer;
        Fill_Routine_Pointer : Fill_Polygon_Procedure_Pointer_Type;
        Preserve_Rectangles_As_Integer : Integer;
    begin
        Transformation_Data_Address := Transformation_Data'Address;

        -- If there ever become available other fill routines, this pointer
        -- assignment will have to be handled by the caller and the function
        -- pointer passed as an argument, breaking backward compatibility a little.
        Fill_Routine_Pointer := plfillada'access;

        if Preserve_Rectangles then
            Preserve_Rectangles_As_Integer := 1;
        else
            Preserve_Rectangles_As_Integer := 0;
        end if;
        
        plshade(Matrix_To_Pointers(z), z'Length(1), z'Length(2), Mask_Function_Pointer,
            x_Min, x_Max, y_Min, y_Max, Shade_Min, Shade_Max, Select_Color_Map, 
            Color, Fill_Pattern_Pen_Width, 
            Shade_Min_Pen_Color, Shade_Min_Pen_Width, 
            Shade_Max_Pen_Color, Shade_Max_Pen_Width, 
            Fill_Routine_Pointer, Preserve_Rectangles_As_Integer,
            Transformation_Procedure_Pointer, Transformation_Data_Address);
    end Shade_Region;


    -- plshade1
    procedure Shade_Region_1
       (z                                        : Real_Matrix;
        Mask_Function_Pointer                    : Mask_Function_Pointer_Type;
        x_Min, x_Max, y_Min, y_Max               : Long_Float; -- world mins and maxes
        Shade_Min, Shade_Max                     : Long_Float;
        Select_Color_Map                         : Natural; -- should be 0 or 1
        Color                                    : Long_Float;
        Fill_Pattern_Pen_Width                   : Positive;
        Shade_Min_Pen_Color, Shade_Min_Pen_Width : Natural;
        Shade_Max_Pen_Color, Shade_Max_Pen_Width : Natural;
        Preserve_Rectangles                      : Boolean;
        Transformation_Procedure_Pointer         : Transformation_Procedure_Pointer_Type;
        Transformation_Data                      : Transformation_Data_Type) is

        Transformation_Data_Address : PLpointer;
        Fill_Routine_Pointer : Fill_Polygon_Procedure_Pointer_Type;
        Preserve_Rectangles_As_Integer : Integer;
    begin
        Transformation_Data_Address := Transformation_Data'Address;

        -- If there ever become available other fill routines, this pointer
        -- assignment will have to be handled by the caller and the function
        -- pointer passed as an argument, breaking backward compatibility a little.
        Fill_Routine_Pointer := plfillada'access;

        if Preserve_Rectangles then
            Preserve_Rectangles_As_Integer := 1;
        else
            Preserve_Rectangles_As_Integer := 0;
        end if;
        
        plshade1(z, z'Length(1), z'Length(2), Mask_Function_Pointer,
            x_Min, x_Max, y_Min, y_Max, Shade_Min, Shade_Max, Select_Color_Map, 
            Color, Fill_Pattern_Pen_Width, 
            Shade_Min_Pen_Color, Shade_Min_Pen_Width, 
            Shade_Max_Pen_Color, Shade_Max_Pen_Width, 
            Fill_Routine_Pointer, Preserve_Rectangles_As_Integer,
            Transformation_Procedure_Pointer, Transformation_Data_Address);
    end Shade_Region_1;


    -- plshades
    procedure Shade_Regions
       (z                                : Real_Matrix;
        Mask_Function_Pointer            : Mask_Function_Pointer_Type;
        x_Min, x_Max, y_Min, y_Max       : Long_Float; -- world mins and maxes
        Contour_Levels                   : Real_Vector;
        Fill_Pattern_Pen_Width           : Positive;
        Contour_Pen_Color                : Natural; -- 0 for no contours
        Contour_Pen_Width                : Natural; -- 0 for no contours
        Preserve_Rectangles              : Boolean;
        Transformation_Procedure_Pointer : Transformation_Procedure_Pointer_Type;
        Transformation_Data              : Transformation_Data_Type) is

        Transformation_Data_Address : PLpointer;
        Fill_Routine_Pointer : Fill_Polygon_Procedure_Pointer_Type;
        Preserve_Rectangles_As_Integer : Integer;
    begin
        Transformation_Data_Address := Transformation_Data'Address;

        -- If there ever become available other fill routines, this pointer
        -- assignment will have to be handled by the caller and the function
        -- pointer passed as an argument, breaking backward compatibility a little.
        Fill_Routine_Pointer := plfillada'access;

        if Preserve_Rectangles then
            Preserve_Rectangles_As_Integer := 1;
        else
            Preserve_Rectangles_As_Integer := 0;
        end if;
        
        plshades(Matrix_To_Pointers(z), z'Length(1), z'Length(2), Mask_Function_Pointer, 
            x_Min, x_Max, y_Min, y_Max, Contour_Levels, Contour_Levels'Length, 
            Fill_Pattern_Pen_Width, Contour_Pen_Color, Contour_Pen_Width, 
            Fill_Routine_Pointer, Preserve_Rectangles_As_Integer, 
            Transformation_Procedure_Pointer, Transformation_Data_Address);
    end Shade_Regions;

    
    -- The procedure plfshade is not part of the API. If it should be necessary 
    -- to make it available to Ada programs, use the binding to plfcont in this 
    -- file as a guidline.

    -- fix this
    --    procedure
    --    plfshade(PLFLT (*f2eval) ( : PLINT;  : PLINT; PLPointer),
    --         PLPointer f2eval_data,
    --         PLFLT (*c2eval) ( : PLINT;  : PLINT; PLPointer),
    --         PLPointer c2eval_data,
    --         nx : PLINT; ny : PLINT;
    --         left : PLFLT; right : PLFLT; bottom : PLFLT; top : PLFLT;
    --         shade_min : PLFLT; shade_max : PLFLT;
    --         sh_cmap : PLINT; sh_color : PLFLT; sh_width : PLINT;
    --         min_color : PLINT; min_width : PLINT;
    --         max_color : PLINT; max_width : PLINT;
    --         void (*fill) ( : PLINT;  : PL_Float_Array;  : PL_Float_Array), rectangular : PLINT;
    --         void (*pltr) ( : PLFLT;  : PLFLT;  : PL_Float_Array;  : PL_Float_Array; PLPointer),
    --         PLPointer pltr_data);


    -- Set up lengths of major tick marks.
    -- plsmaj
    procedure Set_Major_Tick_Length(Default_Length, Scale_Factor : Long_Float) is
    begin
        plsmaj(Default_Length, Scale_Factor);
    end Set_Major_Tick_Length;


    -- Set the memory area to be plotted (with the 'mem' driver)
    -- plsmem
    procedure Plot_From_Memory
       (x_Size, y_Size : Integer;
        Plot_This      : System.Address) is
    begin
        plsmem(x_Size, y_Size, Plot_This);
    end Plot_From_Memory;


    -- Set up lengths of minor tick marks.
    -- plsmin
    procedure Set_Minor_Tick_Length(Default_Length, Scale_Factor : Long_Float) is
    begin
        plsmin(Default_Length, Scale_Factor);
    end Set_Minor_Tick_Length;


    -- Set orientation. Must be done before calling plinit.
    -- plsori
    procedure Set_Orientation(Orientation : Orientation_Type) is
    begin
        if Orientation = Landscape then
            plsori(0);
        else -- Orientation = Portrait
            plsori(1);
        end if;
    end Set_Orientation;


    -- Set output device parameters. Usually ignored by the driver.
    -- plspage
    procedure Set_Page_Parameters
       (x_Pixels, y_Pixels : Long_Float;
        x_Length, y_Length : Integer;
        x_Offset, y_Offset : Integer) is
    begin
        plspage(x_Pixels, y_Pixels, x_Length, y_Length, x_Offset, y_Offset);
    end Set_Page_Parameters;


    -- Set the pause (on end-of-page) status
    -- plspause
    procedure Set_Pause(Pause : Boolean) is
    
        PL_Pause : PLBOOL;
    
    begin
        if Pause then
            PL_Pause := PLtrue;
        else
            PL_Pause := PLfalse;
        end if;
        plspause(PL_Pause);
    end Set_Pause;


    -- Set stream number.
    -- plsstrm
    procedure Set_Stream_Number(Stream_Number : Integer) is
    begin
        plsstrm(Stream_Number);
    end Set_Stream_Number;


    -- Set the number of subwindows in x and y
    -- plssub
    procedure Set_Number_Of_Subpages(x_Number, y_Number : Integer) is
    begin
        plssub(x_Number, y_Number);
    end Set_Number_Of_Subpages;


    -- Set symbol height.
    -- plssym
    procedure Set_Symbol_Size(Default_Height, Scale_Factor : Long_Float) is
    begin
        plssym(Default_Height, Scale_Factor);
    end Set_Symbol_Size;


    -- Initialize PLplot, passing in the windows/page settings.
    -- plstar
    procedure Initialize_Query_Device
       (Number_Horizontal_Subpages, Number_Vertical_Subpages : Integer := 1) is
    begin
        plstar(Number_Horizontal_Subpages, Number_Vertical_Subpages);
    end Initialize_Query_Device;


    -- Initialize PLplot, passing the device name and windows/page settings.
    -- plstart
    procedure Initialize_Set_Device
       (Device_Name                                          : String;
        Number_Horizontal_Subpages, Number_Vertical_Subpages : Integer := 1) is
    begin
        plstart(To_C(Device_Name), Number_Horizontal_Subpages, Number_Vertical_Subpages);
    end Initialize_Set_Device;


    -- Add a point to a stripchart.
    -- plstripa
    procedure Update_Stripchart
       (ID         : Integer;
        Pen_Number : Integer;
        x, y       : Long_Float) is
    begin
        plstripa(ID, Pen_Number, x, y);
    end Update_Stripchart;


    -- Create 1d stripchart
    -- plstripc
    procedure Create_Stripchart
       (ID                                   : out Integer;
        X_Options, Y_Options                 : String;
        x_Min, x_Max                         : Long_Float;
        x_Jump                               : Long_Float;
        y_Min, y_Max                         : Long_Float;
        x_Legend_Position, y_Legend_Position : Long_Float;
        Autoscale_Y                          : Boolean;
        Accumulate                           : Boolean; --Accumulate or slide
        Box_Color, Legend_Color              : Plot_Color_Type;
        Pen_Colors                           : Integer_Array_1D;
        Line_Styles                          : Integer_Array_1D;
        Pen_Labels                           : Stripchart_Label_String_Array_Type;
        X_Label, Y_Label, Title_Label        : String := To_String(Default_Label_String)) is
    
        PL_Autoscale_Y, PL_Accumulate : PLBOOL;
        PL_Pen_Labels : Stripchart_String_Array;
        
        x_LP : Long_Float renames x_Legend_Position;
        y_LP : Long_Float renames y_Legend_Position;
        
    begin
        if Autoscale_Y then
            PL_Autoscale_Y := PLtrue;
        else
            PL_Autoscale_Y := PLfalse;
        end if;
        
        if Accumulate then
            PL_Accumulate := PLtrue;
        else
            PL_Accumulate := PLfalse;
        end if;
        
        -- Adapt strings for Pen_Labels to C.
        for Index in 1..4 loop
            PL_Pen_Labels(Index) := To_C(To_String(Pen_Labels(Index)), True);
        end loop;

        plstripc(ID, To_C(X_Options), To_C(Y_Options), x_Min, x_Max, x_Jump, y_Min, y_Max, x_LP, y_LP, PL_Autoscale_Y, PL_Accumulate, Box_Color, Legend_Color, Pen_Colors, Line_Styles, PL_Pen_Labels, To_C(x_Label), To_C(y_Label), To_C(Title_Label));
    end Create_Stripchart;


    -- Deletes and releases memory used by a stripchart.
    -- plstripd
    procedure Delete_Stripchart(ID : Integer) is
    begin
        plstripd(ID);
    end Delete_Stripchart;


    -- plots a 2d image (or a matrix too large for plshade() )
    -- plimage
    procedure Draw_Image -- No documentation in Chapter 17 of Programmer's Reference Manual
       (Data : Real_Matrix;
        x_Min, x_Max : Long_Float;
        y_Min, y_Max : Long_Float;
        z_Min, z_Max : Long_Float;
        Dxmin, Dxmax : Long_Float;
        Dymin, Dymax : Long_Float) is
    begin
        plimage(Matrix_To_Pointers(Data), Data'Length(2), Data'Length(1), x_Min, x_Max, y_Min, y_Max, z_Min, z_Max, Dxmin, Dxmax, Dymin, Dymax);
    end Draw_Image;


    -- Set up a new line style
    -- plstyl
    procedure Set_Line_Style(Marks, Spaces : Integer_Array_1D) is
    begin
        plstyl(Marks'Length, Marks, Spaces);
    end Set_Line_Style;

    -- This is an overloaded procedure equivalent to calling plstyl with its
    -- first argument zero.
    -- plstyl
    procedure Set_Line_Style(Default_Continuous_Line : Integer) is
        Dummy_Marks, Dummy_Spaces : Integer_Array_1D(1 .. 1) := (others => 0);
    begin
        plstyl(0, Dummy_Marks, Dummy_Spaces);
    end Set_Line_Style;


    -- Plots the 3d surface representation of the function z[x][y].
    -- plsurf3d
    procedure Shaded_Surface_3D
       (x, y           : Real_Vector; -- surface definition points
        z              : Real_Matrix; -- height of surface at definition points
        Options        : Integer;
        Contour_Levels : Real_Vector) is -- levels at which to draw contours        
    begin
        plsurf3d(x, y, Matrix_To_Pointers(z), x'Length, y'Length, Options, Contour_Levels, Contour_Levels'Length);
    end Shaded_Surface_3D;


    -- Plots the 3d surface representation of the function z[x][y] with y
    -- index limits.
    -- plsurf3dl


    -- Sets the edges of the viewport to the specified absolute coordinates
    -- plsvpa
    procedure Set_Viewport_Absolute
       (Left_Edge   : Long_Float;    -- millimeters from edge of subpage
        Right_Edge  : Long_Float;    -- millimeters from edge of subpage
        Bottom_Edge : Long_Float;    -- millimeters from edge of subpage
        Top_Edge    : Long_Float) is -- millimeters from edge of subpage
    begin
        plsvpa(Left_Edge, Right_Edge, Bottom_Edge, Top_Edge);
    end Set_Viewport_Absolute;


    -- Set x axis labeling parameters
    -- "digits" changed to "field_digits".
    -- plsxax
    procedure Set_Floating_Point_Display_X(Max_Digits, Field_Digits : Integer) is
    begin
        plsxax(Max_Digits, Field_Digits);
    end Set_Floating_Point_Display_X;


    -- fix this; undocumented in Chapter 17.
    -- Set inferior X window
    -- plsxwin
    procedure Set_Inferior_Window(Window_ID : Integer) is
    begin
        plsxwin(Window_ID);
    end Set_Inferior_Window;
    

    -- Set y axis labeling parameters
    -- "digits" changed to "field_digits".
    -- plsyax
    procedure Set_Floating_Point_Display_Y(Max_Digits, Field_Digits : Integer) is
    begin
        plsyax(Max_Digits, Field_Digits);
    end Set_Floating_Point_Display_Y;


    -- Plots array y against x for n points using Hershey symbol "code"
    -- plsym
    procedure Draw_Hershey_Symbol
       (x, y : Real_Vector;
        Hershey_Code : Integer) is
    begin
        plsym(x'Length, x, y, Hershey_Code);
    end Draw_Hershey_Symbol;


    -- Set z axis labeling parameters
    -- "digits" changed to "field_digits".
    -- plszax
    procedure Set_Floating_Point_Display_Z(Max_Digits, Field_Digits : Integer) is
    begin
        plszax(Max_Digits, Field_Digits);
    end Set_Floating_Point_Display_Z;


    -- Switches to text screen.
    -- pltext
    procedure Use_Text_Mode is
    begin
        pltext;
    end Use_Text_Mode;


    -- Sets the edges of the viewport with the given aspect ratio, leaving
    -- room for labels.
    -- plvasp
    procedure Set_Viewport_Aspect_Ratio(Aspect_Ratio : Long_Float) is
    begin
        plvasp(Aspect_Ratio);
    end Set_Viewport_Aspect_Ratio;


    -- Creates the largest viewport of the specified aspect ratio that fits
    -- within the specified normalized subpage coordinates.
    -- plvpas
    procedure Set_Viewport_Maximized_For_Aspect_Ratio
       (x_Min, x_Max : Long_Float;
        y_Min, y_Max : Long_Float;
        Aspect_Ratio : Long_Float) is
    begin
        plvpas(x_Min, x_Max, y_Min, y_Max, Aspect_Ratio);
    end Set_Viewport_Maximized_For_Aspect_Ratio;


    -- Creates a viewport with the specified normalized subpage coordinates.
    -- plvpor
    procedure Set_Viewport_Normalized
       (Left_Edge   : Long_Float := 0.0;
        Right_Edge  : Long_Float := 1.0;
        Bottom_Edge : Long_Float := 0.0;
        Top_Edge    : Long_Float := 1.0) is
    begin
        plvpor(Left_Edge, Right_Edge, Bottom_Edge, Top_Edge);
    end Set_Viewport_Normalized;


    -- Defines a "standard" viewport with seven character heights for
    -- the left margin and four character heights everywhere else.
    -- plvsta
    procedure Set_Viewport_Standard is
    begin
        plvsta;
    end Set_Viewport_Standard;


    -- Set up a window for three-dimensional plotting.
    -- plw3d
    procedure Set_Up_3D
       (X_Box, Y_Box, Z_Box                      : Long_Float; -- Extents of enclosing box; world coordinates
        X_Min, X_Max, Y_Min, Y_Max, Z_Min, Z_Max : Long_Float; -- Data limits; user coordinates
        Altitude, Azimuth                        : Long_Float) is -- Viewing angles in world coordinates
    begin
        plw3d(X_Box, Y_Box, Z_Box, X_Min, X_Max, Y_Min, Y_Max, Z_Min, Z_Max, Altitude, Azimuth);
    end Set_Up_3D;


    -- Set pen width.
    -- plwid
    procedure Set_Pen_Width(Pen_Width : Integer) is
    begin
        plwid(Pen_Width);
    end Set_Pen_Width;


    -- Set up world coordinates of the viewport boundaries (2d plots).
    -- plwind
    procedure Set_Viewport_World
       (Left_Edge   : Long_Float;
        Right_Edge  : Long_Float;
        Bottom_Edge : Long_Float;
        Top_Edge    : Long_Float) is
    begin
        plwind(Left_Edge, Right_Edge, Bottom_Edge, Top_Edge);
    end Set_Viewport_World;


    -- set xor mode; mode = 1-enter, 0-leave, status = 0 if not interactive device 
    -- plxormod
    procedure Set_XOR_Mode
       (Use_XOR : Boolean;
        Supports_XOR : out Boolean) is
        
        PL_Use_XOR, PL_Supports_XOR : PLBOOL;
        
    begin
        if Use_XOR then
            PL_Use_XOR := PLtrue;
        else
            PL_Use_XOR := PLfalse;
        end if;
        
        plxormod(PL_Use_XOR, PL_Supports_XOR);
        
        if PL_Supports_XOR = PLtrue then
            Supports_XOR := True;
        elsif PL_Supports_XOR = PLfalse then
            Supports_XOR := False;
        else
            Put_Line("Invalid driver status returned from PLplot in Set_XOR_Mode.");
        end if;
    end Set_XOR_Mode;


--------------------------------------------------------------------------------
--         Functions for use from C or C++ only                               --
--         (Not really ;).                                                    --
--------------------------------------------------------------------------------
--  THESE FUNCTIONS ^^^ ARE NOT IMPLEMENTED FOR THE ADA BINDING
--  EXCEPT FOR THE FOLLOWING.

    -- This procedure is currently GNAT-specific, importing Gnat_Argc and Gnat_Argv.
    -- Process options list using current options info.
    -- plparseopts
    procedure Parse_Command_Line_Arguments(Mode : Parse_Mode_Type) is

        Gnat_Argc : aliased Integer;
        pragma Import (C, Gnat_Argc);

        Gnat_Argv : System.Address;
        pragma Import (C, Gnat_Argv);

        type Gnat_Argc_Access_Type is access all Integer;
        Gnat_Argc_Access : Gnat_Argc_Access_Type;

        procedure
        plparseopts_local(argc : Gnat_Argc_Access_Type;
                          argv : System.Address;
                          mode : Parse_Mode_Type);
        pragma Import(C, plparseopts_local, "c_plparseopts");

    begin
        Gnat_Argc_Access := Gnat_Argc'access;
        plparseopts_local(Gnat_Argc_Access, Gnat_Argv, Mode);
    end Parse_Command_Line_Arguments;


    -- Process input strings, treating them as an option and argument pair.
    -- plsetopt
    procedure Set_Command_Line_Option(Option, Argument : String) is
    begin
        plsetopt(To_C(Option, True), To_C(Argument, True));
    end Set_Command_Line_Option;


	-- Transformation routines

    -- These wrappers are necessary because Ada pointers don't like to point to 
    -- subroutines having non-Ada conventions (I suppose).

    -- Identity transformation.
    -- pltr0
    procedure Plot_Transformation_0
       (x_Grid, y_Grid   : Long_Float;
        x_World, y_World : out Long_Float;
        Data             : PLpointer) is
    begin
        pltr0(x_Grid, y_Grid, x_World, y_World, Data);
    end Plot_Transformation_0;
        

    -- Does linear interpolation from singly dimensioned coord arrays.
    -- pltr1
    procedure Plot_Transformation_1
       (x_Grid, y_Grid   : Long_Float;
        x_World, y_World : out Long_Float;
        Data_Pointer     : PLpointer) is
    begin
        pltr1(x_Grid, y_Grid, x_World, y_World, Data_Pointer);
    end Plot_Transformation_1;


    -- Does linear interpolation from doubly dimensioned coord arrays
    -- (column dominant, as per normal C 2d arrays).
    -- pltr2
    procedure Plot_Transformation_2
       (x_Grid, y_Grid   : Long_Float;
        x_World, y_World : out Long_Float;
        Data_Pointer     : PLpointer) is
    begin
        pltr2(x_Grid, y_Grid, x_World, y_World, Data_Pointer);
    end Plot_Transformation_2;


begin -- package body for PLplot

    -- NOTE: One would ordinarily call the initialization procedure at this point.
    -- However, PLplot has several optional procedures that, if used, must be
    -- called _before_ the initializaiton, here called Initialize_PLplot.
    -- Therefore, Initialize_PLplot MUST BE CALLED FROM THE USER'S PROGRAM, 
    -- before beginning normal plotting but AFTER any optional pre-initialization.

    -- The following lines are some examples of how the entire initialzation 
    -- can be done here if the user wants to do so, and is willing to uncomment 
    -- these lines and/or add new lines.


    -- plscompression should be called before plinit when it is needed.
    -- There should be a list of devices which use compression, and plscompression
    -- called conditioned on the current device being in that list.
    
    -- Optionally call plsetopt (Set_Command_Line_Option) before Initialize_PLplot.
    
    -- Parse_Command_Line_Arguments(1);
    -- Set_File_Family_Parameters(False); -- Default off; other args are don't-cares here.
    -- Set_Orientation(Landscape); -- Optional; before Initialize_PLplot if used.

    -- Initialize_PLplot;


    -- Capture the initial, default, settings of color map 0 since these will be  
    -- lost if the settings for color map 0 are set by the user. They can be 
    -- restored collectively by calling Restore_Default_Snapshot_Of_Color_Map_0 
    -- or individually by calling functions such as Reset_Red etc. for each of 
    -- the 16 default colors of color map 0.
    Make_Snapshot_Of_Color_Map_0(Default_Red_Components, Default_Green_Components, Default_Blue_Components);

end PLplot;
