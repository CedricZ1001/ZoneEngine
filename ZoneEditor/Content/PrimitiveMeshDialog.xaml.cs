﻿using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using ZoneEditor.ContentToolsAPIStructs;
using ZoneEditor.DllWrappers;
using ZoneEditor.Editors;
using ZoneEditor.GameProject;
using ZoneEditor.Utilities;
using ZoneEditor.Utilities.Controls;

namespace ZoneEditor.Content
{
    /// <summary>
    /// Interaction logic for PrimitiveMeshDialog.xaml
    /// </summary>
    public partial class PrimitiveMeshDialog : Window
    {

        private static readonly List<ImageBrush> _textures = new List<ImageBrush>();

        private void OnPrimitiveType_ComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e) => UpdatePrimitive();

        private void OnSlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e) => UpdatePrimitive();

        private void OnScalarBox_ValueChanged(object sender, RoutedEventArgs e) => UpdatePrimitive();


        private float Value(ScalarBox scalarBox, float min)
        {
            float.TryParse(scalarBox.Value, out var result);
            return Math.Max(result, min);
        }

        private void UpdatePrimitive()
        {
            if (!IsInitialized) return;

            var primitiveMeshType =(PrimitiveMeshType)PrimitiveMeshTypeComboBox.SelectedItem;
            var info = new PrimitiveInitInfo() { Type = primitiveMeshType };
            var smoothingAngle = 0;

            switch (primitiveMeshType)
            {
                case PrimitiveMeshType.Plane:
                    {
                        info.SegmentX = (int)xSliderPlane.Value;
                        info.SegmentZ = (int)zSliderPlane.Value;
                        info.Size.X = Value(widthScalarBoxPlane, 0.001f);
                        info.Size.Z = Value(lengthScalarBoxPlane, 0.001f);
                        break;
                    }
                case PrimitiveMeshType.Cube:
                    return;
                case PrimitiveMeshType.UVSphere:
                    {
                        info.SegmentX = (int)xSliderUVSphere.Value;
                        info.SegmentY = (int)ySliderUVSphere.Value;
                        info.Size.X = Value(xScalarBoxUVSphere, 0.001f);
                        info.Size.Y = Value(yScalarBoxUVSphere, 0.001f);
                        info.Size.Z = Value(zScalarBoxUVSphere, 0.001f);
                        smoothingAngle = (int)angleSliderUVSphere.Value;
                    }
                    break;
                case PrimitiveMeshType.ICOSphere:
                    return;
                case PrimitiveMeshType.CyLinder:
                    return;
                case PrimitiveMeshType.Capsule:
                    return;
                default:
                    break;
            }

            var geometry = new Geometry();
            geometry.ImportSettings.SmoothingAngle = smoothingAngle;
            ContentToolsAPI.CreatePrimitiveMesh(geometry, info);
            (DataContext as GeometryEditor).SetAsset(geometry);
            OnTexture_CheckBox_Click(textureCheckBox, null);
        }

        private static void LoadTextures()
        {
            var uris = new List<Uri>
            {
                new Uri("pack://application:,,,/Resources/PrimitiveMeshView/PlaneTexture.png"),
                new Uri("pack://application:,,,/Resources/PrimitiveMeshView/PlaneTexture.png"),
                new Uri("pack://application:,,,/Resources/PrimitiveMeshView/UVSphereTexture.png"),
            };

            _textures.Clear();

            foreach (var uri in uris)
            {
                var resource = Application.GetResourceStream(uri);
                if (resource == null)
                {
                    // 没有找到资源，可以记录日志或抛出异常
                    Debug.WriteLine("未能加载资源: " + uri);
                    Logger.Log(MessageType.Warning, "未能加载资源: " + uri);
                    return;
                }
                Logger.Log(MessageType.Info, "加载资源: " + uri);
                using var reader = new BinaryReader(resource.Stream);
                var data = reader.ReadBytes((int)resource.Stream.Length);
                var imageSource = (BitmapSource)new ImageSourceConverter().ConvertFrom(data);
                imageSource.Freeze();

                //var tempFile = Path.Combine(Path.GetTempPath(), "test_image111.png");
                //using (var fileStream = new FileStream(tempFile, FileMode.Create))
                //{
                //    var encoder = new PngBitmapEncoder();
                //    encoder.Frames.Add(BitmapFrame.Create(imageSource));
                //    encoder.Save(fileStream);
                //}
                //Debug.WriteLine($"图像已保存到: {tempFile}");

                var brush = new ImageBrush(imageSource);
                brush.Transform = new ScaleTransform(1, -1, 0.5, 0.5);
                brush.ViewportUnits = BrushMappingMode.Absolute;
                brush.Freeze();
                _textures.Add(brush);
            }
        }

        static PrimitiveMeshDialog()
        {
            LoadTextures();
        }

        public PrimitiveMeshDialog()
        {
            InitializeComponent();
            Loaded += (s, e) => UpdatePrimitive();
        }

        private void OnTexture_CheckBox_Click(object sender, RoutedEventArgs e)
        {
            Brush brush = Brushes.White;
            if((sender as CheckBox).IsChecked == true)
            {
                brush = _textures[(int)PrimitiveMeshTypeComboBox.SelectedIndex];
            }

            var vm = DataContext as GeometryEditor;
            foreach (var mesh in vm.MeshRenderer.Meshes)
            {
                mesh.Diffuse = brush;
            }
        }

        private void OnSave_Button_Click(object sender, RoutedEventArgs e)
        {
            var dlg = new SaveFileDialog()
            {
                InitialDirectory = Project.Current.ContentPath,
                Filter = "Asset file (*.zasset)|*.zasset"
            };

            if (dlg.ShowDialog() == true)
            {
                Debug.Assert(!string.IsNullOrEmpty(dlg.FileName));
                var asset = (DataContext as IAssetEditor).Asset;
                Debug.Assert(asset != null);
                asset.Save(dlg.FileName);
            }
        }
    }
}
