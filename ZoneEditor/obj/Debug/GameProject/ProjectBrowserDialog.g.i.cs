﻿#pragma checksum "..\..\..\GameProject\ProjectBrowserDialog.xaml" "{ff1816ec-aa5e-4d10-87f7-6f4963833460}" "139E05CC91D5094E0341D3C6F773DCAB430A8183"
//------------------------------------------------------------------------------
// <auto-generated>
//     此代码由工具生成。
//     运行时版本:4.0.30319.42000
//
//     对此文件的更改可能会导致不正确的行为，并且如果
//     重新生成代码，这些更改将会丢失。
// </auto-generated>
//------------------------------------------------------------------------------

using System;
using System.Diagnostics;
using System.Windows;
using System.Windows.Automation;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Controls.Ribbon;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Ink;
using System.Windows.Input;
using System.Windows.Markup;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Media.Effects;
using System.Windows.Media.Imaging;
using System.Windows.Media.Media3D;
using System.Windows.Media.TextFormatting;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Shell;
using ZoneEditor.GameProject;


namespace ZoneEditor.GameProject {
    
    
    /// <summary>
    /// ProjectBrowserDialog
    /// </summary>
    public partial class ProjectBrowserDialog : System.Windows.Window, System.Windows.Markup.IComponentConnector {
        
        
        #line 15 "..\..\..\GameProject\ProjectBrowserDialog.xaml"
        [System.Diagnostics.CodeAnalysis.SuppressMessageAttribute("Microsoft.Performance", "CA1823:AvoidUnusedPrivateFields")]
        internal System.Windows.Controls.Primitives.ToggleButton openProjectButton;
        
        #line default
        #line hidden
        
        
        #line 17 "..\..\..\GameProject\ProjectBrowserDialog.xaml"
        [System.Diagnostics.CodeAnalysis.SuppressMessageAttribute("Microsoft.Performance", "CA1823:AvoidUnusedPrivateFields")]
        internal System.Windows.Controls.Primitives.ToggleButton createProjectButton;
        
        #line default
        #line hidden
        
        
        #line 20 "..\..\..\GameProject\ProjectBrowserDialog.xaml"
        [System.Diagnostics.CodeAnalysis.SuppressMessageAttribute("Microsoft.Performance", "CA1823:AvoidUnusedPrivateFields")]
        internal System.Windows.Controls.StackPanel browserContent;
        
        #line default
        #line hidden
        
        private bool _contentLoaded;
        
        /// <summary>
        /// InitializeComponent
        /// </summary>
        [System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [System.CodeDom.Compiler.GeneratedCodeAttribute("PresentationBuildTasks", "8.0.8.0")]
        public void InitializeComponent() {
            if (_contentLoaded) {
                return;
            }
            _contentLoaded = true;
            System.Uri resourceLocater = new System.Uri("/ZoneEditor;V1.0.0.0;component/gameproject/projectbrowserdialog.xaml", System.UriKind.Relative);
            
            #line 1 "..\..\..\GameProject\ProjectBrowserDialog.xaml"
            System.Windows.Application.LoadComponent(this, resourceLocater);
            
            #line default
            #line hidden
        }
        
        [System.Diagnostics.DebuggerNonUserCodeAttribute()]
        [System.CodeDom.Compiler.GeneratedCodeAttribute("PresentationBuildTasks", "8.0.8.0")]
        [System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)]
        [System.Diagnostics.CodeAnalysis.SuppressMessageAttribute("Microsoft.Design", "CA1033:InterfaceMethodsShouldBeCallableByChildTypes")]
        [System.Diagnostics.CodeAnalysis.SuppressMessageAttribute("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity")]
        [System.Diagnostics.CodeAnalysis.SuppressMessageAttribute("Microsoft.Performance", "CA1800:DoNotCastUnnecessarily")]
        void System.Windows.Markup.IComponentConnector.Connect(int connectionId, object target) {
            switch (connectionId)
            {
            case 1:
            this.openProjectButton = ((System.Windows.Controls.Primitives.ToggleButton)(target));
            
            #line 16 "..\..\..\GameProject\ProjectBrowserDialog.xaml"
            this.openProjectButton.Click += new System.Windows.RoutedEventHandler(this.OnToggleButton_Click);
            
            #line default
            #line hidden
            return;
            case 2:
            this.createProjectButton = ((System.Windows.Controls.Primitives.ToggleButton)(target));
            
            #line 18 "..\..\..\GameProject\ProjectBrowserDialog.xaml"
            this.createProjectButton.Click += new System.Windows.RoutedEventHandler(this.OnToggleButton_Click);
            
            #line default
            #line hidden
            return;
            case 3:
            this.browserContent = ((System.Windows.Controls.StackPanel)(target));
            return;
            }
            this._contentLoaded = true;
        }
    }
}

