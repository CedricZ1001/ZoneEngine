using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ZoneEditor.Editors
{
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
