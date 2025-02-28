using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Media;
using System.Windows.Media.Media3D;

namespace ZoneEditor.Editors
{
    class MeshRendererVertexData : ViewModelBase
    {
        
        private Brush _specular = new SolidColorBrush((Color)ColorConverter.ConvertFromString("#ff111111"));
        public Brush Specular
        {
            get => _specular;
            set
            {
                if (_specular != value)
                {
                    _specular = value;
                    OnPropertyChanged(nameof(Specular));
                }
            }
        }

        private Brush _diffuse = Brushes.White;
        public Brush Diffuse
        {
            get => _diffuse;
            set
            {
                if (_diffuse != value)
                {
                    _specular = value;
                    OnPropertyChanged(nameof(Diffuse));
                }
            }
        }


        public Point3DCollection Position { get; } = new Point3DCollection();
        public Vector3DCollection Normals { get; } = new Vector3DCollection();
        public PointCollection UVs { get; } = new PointCollection();
        public Int32Collection Indices { get; } = new Int32Collection();

    }
    // NOTE: 


    class GeometryEditor : ViewModelBase, IAssetEditor
    {
        public Content.Asset Asset => Geometry;


        private Content.Geometry _geometry;
        public Content.Geometry Geometry
        {
            get => _geometry;
            set
            {
                if (_geometry != value)
                {
                    _geometry = value;
                    OnPropertyChanged(nameof(Geometry));
                }
            }
        }


        public void SetAsset(Content.Asset asset)
        {
            Debug.Assert(asset is Content.Geometry);
            if (asset is Content.Geometry geometry)
            {
                Geometry = geometry;
            }
        }
    }
}
