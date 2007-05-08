// -*-c++-*-

#include <osgViewer/Viewer>

#include <osgDB/ReadFile>

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/StateSet>
#include <osg/Material>
#include <osg/Texture2D>
#include <osg/TextureRectangle>
#include <osg/TextureCubeMap>
#include <osg/TexMat>
#include <osg/CullFace>
#include <osg/ImageStream>
#include <osg/io_utils>

#include <osgGA/TrackballManipulator>
#include <osgGA/EventVisitor>

#include <iostream>

osg::ImageStream* s_imageStream = 0;
class MovieEventHandler : public osgGA::GUIEventHandler
{
public:

    MovieEventHandler():_trackMouse(false) {}
    
    void setMouseTracking(bool track) { _trackMouse = track; }
    bool getMouseTracking() const { return _trackMouse; }
    
    void set(osg::Node* node);

    virtual bool handle(const osgGA::GUIEventAdapter& ea,osgGA::GUIActionAdapter& aa, osg::Object*, osg::NodeVisitor* nv);
    
    virtual void getUsage(osg::ApplicationUsage& usage) const;

    typedef std::vector< osg::ref_ptr<osg::ImageStream> > ImageStreamList;

protected:

    virtual ~MovieEventHandler() {}

    class FindImageStreamsVisitor : public osg::NodeVisitor
    {
    public:
        FindImageStreamsVisitor(ImageStreamList& imageStreamList):
            _imageStreamList(imageStreamList) {}
            
        virtual void apply(osg::Geode& geode)
        {
            apply(geode.getStateSet());

            for(unsigned int i=0;i<geode.getNumDrawables();++i)
            {
                apply(geode.getDrawable(i)->getStateSet());
            }
        
            traverse(geode);
        }

        virtual void apply(osg::Node& node)
        {
            apply(node.getStateSet());
            traverse(node);
        }
        
        inline void apply(osg::StateSet* stateset)
        {
            if (!stateset) return;
            
            osg::StateAttribute* attr = stateset->getTextureAttribute(0,osg::StateAttribute::TEXTURE);
            if (attr)
            {
                osg::Texture2D* texture2D = dynamic_cast<osg::Texture2D*>(attr);
                if (texture2D) apply(dynamic_cast<osg::ImageStream*>(texture2D->getImage()));

                osg::TextureRectangle* textureRec = dynamic_cast<osg::TextureRectangle*>(attr);
                if (textureRec) apply(dynamic_cast<osg::ImageStream*>(textureRec->getImage()));
            }
        }
        
        inline void apply(osg::ImageStream* imagestream)
        {
            if (imagestream)
            {
                _imageStreamList.push_back(imagestream); 
                s_imageStream = imagestream;
            }
        }
        
        ImageStreamList& _imageStreamList;
    };


    bool            _trackMouse;
    ImageStreamList _imageStreamList;
    
};



void MovieEventHandler::set(osg::Node* node)
{
    _imageStreamList.clear();
    if (node)
    {
        FindImageStreamsVisitor fisv(_imageStreamList);
        node->accept(fisv);
    }
}


bool MovieEventHandler::handle(const osgGA::GUIEventAdapter& ea,osgGA::GUIActionAdapter& aa, osg::Object*, osg::NodeVisitor* nv)
{
    switch(ea.getEventType())
    {
        case(osgGA::GUIEventAdapter::MOVE):
        case(osgGA::GUIEventAdapter::PUSH):
        case(osgGA::GUIEventAdapter::RELEASE):
        {
            if (_trackMouse)
            {
                osgViewer::View* view = dynamic_cast<osgViewer::View*>(&aa);
                osgUtil::LineSegmentIntersector::Intersections intersections;
                if (view && view->computeIntersections(ea.getX(), ea.getY(), nv->getNodePath(), intersections))
                {

                    // use the nearest intersection                 
                    const osgUtil::LineSegmentIntersector::Intersection& intersection = *(intersections.begin());
                    osg::Drawable* drawable = intersection.drawable.get();
                    osg::Geometry* geometry = drawable ? drawable->asGeometry() : 0;
                    osg::Vec3Array* vertices = geometry ? dynamic_cast<osg::Vec3Array*>(geometry->getVertexArray()) : 0;
                    if (vertices)
                    {
                        // get the vertex indices.
                        const osgUtil::LineSegmentIntersector::Intersection::IndexList& indices = intersection.indexList;
                        const osgUtil::LineSegmentIntersector::Intersection::RatioList& ratios = intersection.ratioList;

                        if (indices.size()==3 && ratios.size()==3)
                        {
                            unsigned int i1 = indices[0];
                            unsigned int i2 = indices[1];
                            unsigned int i3 = indices[2];

                            float r1 = ratios[0];
                            float r2 = ratios[1];
                            float r3 = ratios[2];

                            osg::Array* texcoords = (geometry->getNumTexCoordArrays()>0) ? geometry->getTexCoordArray(0) : 0;
                            osg::Vec2Array* texcoords_Vec2Array = dynamic_cast<osg::Vec2Array*>(texcoords);
                            if (texcoords_Vec2Array)
                            {
                                // we have tex coord array so now we can compute the final tex coord at the point of intersection.                                
                                osg::Vec2 tc1 = (*texcoords_Vec2Array)[i1];
                                osg::Vec2 tc2 = (*texcoords_Vec2Array)[i2];
                                osg::Vec2 tc3 = (*texcoords_Vec2Array)[i3];
                                osg::Vec2 tc = tc1*r1 + tc2*r2 + tc3*r3;

                                osg::notify(osg::NOTICE)<<"We hit tex coords "<<tc<<std::endl;

                            }
                        }
                        else
                        {
                            osg::notify(osg::NOTICE)<<"Intersection has insufficient indices to work with";
                        }

                    }
                }
                else
                {
                    osg::notify(osg::NOTICE)<<"No intersection"<<std::endl;
                }
            }
            break;
        }
        case(osgGA::GUIEventAdapter::KEYDOWN):
        {
            if (ea.getKey()=='s')
            {
                for(ImageStreamList::iterator itr=_imageStreamList.begin();
                    itr!=_imageStreamList.end();
                    ++itr)
                {
                    std::cout<<"Play"<<std::endl;
                     (*itr)->play();
                }
                return true;
            }
            else if (ea.getKey()=='p')
            {
                for(ImageStreamList::iterator itr=_imageStreamList.begin();
                    itr!=_imageStreamList.end();
                    ++itr)
                {
                    std::cout<<"Pause"<<std::endl;
                    (*itr)->pause();
                }
                return true;
            }
            else if (ea.getKey()=='r')
            {
                for(ImageStreamList::iterator itr=_imageStreamList.begin();
                    itr!=_imageStreamList.end();
                    ++itr)
                {
                    std::cout<<"Restart"<<std::endl;
                    (*itr)->rewind();
                    (*itr)->play();
                }
                return true;
            }
            else if (ea.getKey()=='l')
            {
                for(ImageStreamList::iterator itr=_imageStreamList.begin();
                    itr!=_imageStreamList.end();
                    ++itr)
                {
                    if ( (*itr)->getLoopingMode() == osg::ImageStream::LOOPING)
                    {
                        std::cout<<"Toggle Looping Off"<<std::endl;
                        (*itr)->setLoopingMode( osg::ImageStream::NO_LOOPING );
                    }
                    else
                    {
                        std::cout<<"Toggle Looping On"<<std::endl;
                        (*itr)->setLoopingMode( osg::ImageStream::LOOPING );
                    }
                }
                return true;
            }
            return false;
        }

        default:
            return false;
    }
    return false;
}

void MovieEventHandler::getUsage(osg::ApplicationUsage& usage) const
{
    usage.addKeyboardMouseBinding("p","Pause movie");
    usage.addKeyboardMouseBinding("s","Play movie");
    usage.addKeyboardMouseBinding("r","Restart movie");
    usage.addKeyboardMouseBinding("l","Toggle looping of movie");
}


osg::Geometry* myCreateTexturedQuadGeometry(const osg::Vec3& pos,float width,float height, osg::Image* image, bool useTextureRectangle)
{
    if (useTextureRectangle)
    {
        osg::Geometry* pictureQuad = osg::createTexturedQuadGeometry(pos,
                                           osg::Vec3(width,0.0f,0.0f),
                                           osg::Vec3(0.0f,0.0f,height),
                                           0.0f,image->t(), image->s(),0.0f);

        pictureQuad->getOrCreateStateSet()->setTextureAttributeAndModes(0,
                    new osg::TextureRectangle(image),
                    osg::StateAttribute::ON);
                    
        return pictureQuad;
    }
    else
    {
        osg::Geometry* pictureQuad = osg::createTexturedQuadGeometry(pos,
                                           osg::Vec3(width,0.0f,0.0f),
                                           osg::Vec3(0.0f,0.0f,height),
                                           0.0f,1.0f, 1.0f,0.0f);
                                    
        osg::Texture2D* texture = new osg::Texture2D(image);
        texture->setFilter(osg::Texture::MIN_FILTER,osg::Texture::LINEAR);  
                                       
        pictureQuad->getOrCreateStateSet()->setTextureAttributeAndModes(0,
                    texture,
                    osg::StateAttribute::ON);

        return pictureQuad;
    }
}

osg::Geometry* createDomeDistortionMesh(const osg::Vec3& origin, const osg::Vec3& widthVector, const osg::Vec3& heightVector,
                                        osg::ArgumentParser& arguments)
{
    double sphere_radius = 1.0;
    if (arguments.read("--radius", sphere_radius)) {}

    double collar_radius = 0.45;
    if (arguments.read("--collar", collar_radius)) {}

    osg::Vec3d center(0.0,0.0,0.0);
    osg::Vec3d eye(0.0,0.0,0.0);
    
    double distance = sqrt(sphere_radius*sphere_radius - collar_radius*collar_radius);
    if (arguments.read("--distance", distance)) {}
    
    bool centerProjection = false;

    osg::Vec3d projector = eye - osg::Vec3d(0.0,0.0, distance);
    
    
    osg::notify(osg::NOTICE)<<"Projector position = "<<projector<<std::endl;
    osg::notify(osg::NOTICE)<<"distance = "<<distance<<std::endl;


    // create the quad to visualize.
    osg::Geometry* geometry = new osg::Geometry();

    geometry->setSupportsDisplayList(false);

    osg::Vec3 xAxis(widthVector);
    float width = widthVector.length();
    xAxis /= width;

    osg::Vec3 yAxis(heightVector);
    float height = heightVector.length();
    yAxis /= height;
    
    int noSteps = 160;

    osg::Vec3Array* vertices = new osg::Vec3Array;
    osg::Vec2Array* texcoords = new osg::Vec2Array;
    osg::Vec4Array* colors = new osg::Vec4Array;

    osg::Vec3 bottom = origin;
    osg::Vec3 dx = xAxis*(width/((float)(noSteps-2)));
    osg::Vec3 dy = yAxis*(height/((float)(noSteps-1)));
    
    osg::Vec3d screenCenter = origin + widthVector*0.5f + heightVector*0.5f;
    float screenRadius = heightVector.length() * 0.5f;

    osg::Vec3 cursor = bottom;
    int i,j;
    
    int midSteps = noSteps/2;
    
    for(i=0;i<midSteps;++i)
    {
        osg::Vec3 cursor = bottom+dy*(float)i;
        for(j=0;j<midSteps;++j)
        {
            osg::Vec2 delta(cursor.x() - screenCenter.x(), cursor.y() - screenCenter.y());
            double theta = atan2(delta.x(), -delta.y());
            theta += 2*osg::PI;
            double phi = osg::PI_2 * delta.length() / screenRadius;
            if (phi > osg::PI_2) phi = osg::PI_2;

            double f = distance * sin(phi);
            double e = distance * cos(phi) + sqrt( sphere_radius*sphere_radius - f*f);
            double l = e * cos(phi);
            double h = e * sin(phi);
            double gamma = atan2(h, l-distance);

            osg::Vec2 texcoord(theta/(2.0*osg::PI), 1.0-gamma/osg::PI);

            // osg::notify(osg::NOTICE)<<"cursor = "<<cursor<< " theta = "<<theta<< "phi="<<phi<<" gamma = "<<gamma<<" texcoord="<<texcoord<<std::endl;

            vertices->push_back(cursor);
            colors->push_back(osg::Vec4(1.0f,1.0f,1.0f,1.0f));
            texcoords->push_back(texcoord);

            if (j+1<midSteps) cursor += dx;
        }

        for(;j<noSteps;++j)
        {
            osg::Vec2 delta(cursor.x() - screenCenter.x(), cursor.y() - screenCenter.y());
            double theta = atan2(delta.x(), -delta.y());
            double phi = osg::PI_2 * delta.length() / screenRadius;
            if (phi > osg::PI_2) phi = osg::PI_2;

            double f = distance * sin(phi);
            double e = distance * cos(phi) + sqrt( sphere_radius*sphere_radius - f*f);
            double l = e * cos(phi);
            double h = e * sin(phi);
            double gamma = atan2(h, l-distance);

            osg::Vec2 texcoord(theta/(2.0*osg::PI), 1.0-gamma/osg::PI);

            // osg::notify(osg::NOTICE)<<"cursor = "<<cursor<< " theta = "<<theta<< "phi="<<phi<<" gamma = "<<gamma<<" texcoord="<<texcoord<<std::endl;

            vertices->push_back(cursor);
            colors->push_back(osg::Vec4(1.0f,1.0f,1.0f,1.0f));
            texcoords->push_back(texcoord);

            cursor += dx;
        }
        // osg::notify(osg::NOTICE)<<std::endl;
    }
    
    for(;i<noSteps;++i)
    {
        osg::Vec3 cursor = bottom+dy*(float)i;
        for(j=0;j<noSteps;++j)
        {
            osg::Vec2 delta(cursor.x() - screenCenter.x(), cursor.y() - screenCenter.y());
            double theta = atan2(delta.x(), -delta.y());
            if (theta<0.0) theta += 2*osg::PI;
            double phi = osg::PI_2 * delta.length() / screenRadius;
            if (phi > osg::PI_2) phi = osg::PI_2;

            double f = distance * sin(phi);
            double e = distance * cos(phi) + sqrt( sphere_radius*sphere_radius - f*f);
            double l = e * cos(phi);
            double h = e * sin(phi);
            double gamma = atan2(h, l-distance);

            osg::Vec2 texcoord(theta/(2.0*osg::PI), 1.0-gamma/osg::PI);

            // osg::notify(osg::NOTICE)<<"cursor = "<<cursor<< " theta = "<<theta<< "phi="<<phi<<" gamma = "<<gamma<<" texcoord="<<texcoord<<std::endl;

            vertices->push_back(cursor);
            colors->push_back(osg::Vec4(1.0f,1.0f,1.0f,1.0f));
            texcoords->push_back(texcoord);

            cursor += dx;
        }

        // osg::notify(osg::NOTICE)<<std::endl;
    }

    // pass the created vertex array to the points geometry object.
    geometry->setVertexArray(vertices);

    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);

    geometry->setTexCoordArray(0,texcoords);

    for(i=0;i<noSteps-1;++i)
    {
        osg::DrawElementsUShort* elements = new osg::DrawElementsUShort(osg::PrimitiveSet::QUAD_STRIP);
        for(j=0;j<noSteps;++j)
        {
            elements->push_back(j+(i+1)*noSteps);
            elements->push_back(j+(i)*noSteps);
        }
        geometry->addPrimitiveSet(elements);
    }
    
    return geometry;
}


void setDomeCorrection(osgViewer::Viewer& viewer, osg::ArgumentParser& arguments)
{
    // enforce single threading right now to avoid double buffering of RTT texture
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
 
    osg::GraphicsContext::WindowingSystemInterface* wsi = osg::GraphicsContext::getWindowingSystemInterface();
    if (!wsi) 
    {
        osg::notify(osg::NOTICE)<<"Error, no WindowSystemInterface available, cannot create windows."<<std::endl;
        return;
    }

    unsigned int width, height;
    wsi->getScreenResolution(osg::GraphicsContext::ScreenIdentifier(0), width, height);

    while (arguments.read("--width",width)) {}
    while (arguments.read("--height",height)) {}

    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    traits->x = 0;
    traits->y = 0;
    traits->width = width;
    traits->height = height;
    traits->windowDecoration = false;
    traits->doubleBuffer = true;
    traits->sharedContext = 0;
    
    

    osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits.get());
    if (!gc)
    {
        osg::notify(osg::NOTICE)<<"GraphicsWindow has not been created successfully."<<std::endl;
        return;
    }

    osg::ref_ptr<osg::Drawable> distortionCorrectionMash = createDomeDistortionMesh(osg::Vec3(0.0f,0.0f,0.0f), osg::Vec3(width,0.0f,0.0f), osg::Vec3(0.0f,height,0.0f), arguments);

    osg::Texture* texture = 0;
    for(int i=1;i<arguments.argc() && !texture;++i)
    {
        if (arguments.isString(i))
        {
            osg::Image* image = osgDB::readImageFile(arguments[i]);
            osg::ImageStream* imagestream = dynamic_cast<osg::ImageStream*>(image);
            if (imagestream) imagestream->play();

            if (image)
            {
#if 1            
                texture = new osg::TextureRectangle(image);
#else
                texture = new osg::Texture2D(image);
#endif
            }
        }
    }
    
    if (!texture)
    {
        return;
    }

    // distortion correction set up.
    {
        osg::Geode* geode = new osg::Geode();
        geode->addDrawable(distortionCorrectionMash.get());

        // new we need to add the texture to the mesh, we do so by creating a 
        // StateSet to contain the Texture StateAttribute.
        osg::StateSet* stateset = geode->getOrCreateStateSet();
        stateset->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);
        texture->setMaxAnisotropy(16.0f);
        stateset->setMode(GL_LIGHTING,osg::StateAttribute::OFF);
#if 1        
        osg::TexMat* texmat = new osg::TexMat;
        texmat->setScaleByTextureRectangleSize(true);
        stateset->setTextureAttributeAndModes(0, texmat, osg::StateAttribute::ON);
#endif
        osg::ref_ptr<osg::Camera> camera = new osg::Camera;
        camera->setGraphicsContext(gc.get());
        camera->setClearMask(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );
        camera->setClearColor( osg::Vec4(0.1,0.1,1.0,1.0) );
        camera->setViewport(new osg::Viewport(0, 0, width, height));
        GLenum buffer = traits->doubleBuffer ? GL_BACK : GL_FRONT;
        camera->setDrawBuffer(buffer);
        camera->setReadBuffer(buffer);
        camera->setReferenceFrame(osg::Camera::ABSOLUTE_RF);
        camera->setAllowEventFocus(false);
        //camera->setInheritanceMask(camera->getInheritanceMask() & ~osg::CullSettings::CLEAR_COLOR & ~osg::CullSettings::COMPUTE_NEAR_FAR_MODE);
        //camera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
        
        camera->setProjectionMatrixAsOrtho2D(0,width,0,height);
        camera->setViewMatrix(osg::Matrix::identity());

        // add subgraph to render
        // camera->addChild(geode);
        
        camera->setName("DistortionCorrectionCamera");

        viewer.addSlave(camera.get(), osg::Matrixd(), osg::Matrixd(), true);

        viewer.setSceneData(geode);
    }
    
    
    viewer.getCamera()->setNearFarRatio(0.0001f);
}

int main(int argc, char** argv)
{
    // use an ArgumentParser object to manage the program arguments.
    osg::ArgumentParser arguments(&argc,argv);
    
    // set up the usage document, in case we need to print out how to use this program.
    arguments.getApplicationUsage()->setApplicationName(arguments.getApplicationName());
    arguments.getApplicationUsage()->setDescription(arguments.getApplicationName()+" example demonstrates the use of ImageStream for rendering movies as textures.");
    arguments.getApplicationUsage()->setCommandLineUsage(arguments.getApplicationName()+" [options] filename ...");
    arguments.getApplicationUsage()->addCommandLineOption("-h or --help","Display this information");
    arguments.getApplicationUsage()->addCommandLineOption("--texture2D","Use Texture2D rather than TextureRectangle.");
    arguments.getApplicationUsage()->addCommandLineOption("--shader","Use shaders to post process the video.");
    arguments.getApplicationUsage()->addCommandLineOption("--dome","Use full dome distortion correction.");
    
    bool useTextureRectangle = true;
    bool useShader = false;

    // construct the viewer.
    osgViewer::Viewer viewer;
    
    if (arguments.argc()<=1)
    {
        arguments.getApplicationUsage()->write(std::cout,osg::ApplicationUsage::COMMAND_LINE_OPTION);
        return 1;
    }

    while (arguments.read("--texture2D")) useTextureRectangle=false;
    while (arguments.read("--shader")) useShader=true;

    // if user request help write it out to cout.
    if (arguments.read("-h") || arguments.read("--help"))
    {
        arguments.getApplicationUsage()->write(std::cout);
        return 1;
    }



    if (arguments.read("--dome") || arguments.read("--puffer") )
    {    
        setDomeCorrection(viewer, arguments);
    }
    else
    {
        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        osg::Vec3 pos(0.0f,0.0f,0.0f);

        osg::StateSet* stateset = geode->getOrCreateStateSet();
        stateset->setMode(GL_LIGHTING,osg::StateAttribute::OFF);

        if (useShader)
        {
            //useTextureRectangle = false;

            static const char *shaderSourceTextureRec = {
                "uniform vec4 cutoff_color;\n"
                "uniform samplerRect movie_texture;\n"
                "void main(void)\n"
                "{\n"
                "    vec4 texture_color = textureRect(movie_texture, gl_TexCoord[0]); \n"
                "    if (all(lessThanEqual(texture_color,cutoff_color))) discard; \n"
                "    gl_FragColor = texture_color;\n"
                "}\n"
            };

            static const char *shaderSourceTexture2D = {
                "uniform vec4 cutoff_color;\n"
                "uniform sampler2D movie_texture;\n"
                "void main(void)\n"
                "{\n"
                "    vec4 texture_color = texture2D(movie_texture, gl_TexCoord[0]); \n"
                "    if (all(lessThanEqual(texture_color,cutoff_color))) discard; \n"
                "    gl_FragColor = texture_color;\n"
                "}\n"
            };

            osg::Program* program = new osg::Program;

            program->addShader(new osg::Shader(osg::Shader::FRAGMENT,
                                               useTextureRectangle ? shaderSourceTextureRec : shaderSourceTexture2D));

            stateset->addUniform(new osg::Uniform("cutoff_color",osg::Vec4(0.1f,0.1f,0.1f,1.0f)));
            stateset->addUniform(new osg::Uniform("movie_texture",0));

            stateset->setAttribute(program);

        }

        for(int i=1;i<arguments.argc();++i)
        {
            if (arguments.isString(i))
            {
                osg::Image* image = osgDB::readImageFile(arguments[i]);
                osg::ImageStream* imagestream = dynamic_cast<osg::ImageStream*>(image);
                if (imagestream) imagestream->play();

                if (image)
                {
                    geode->addDrawable(myCreateTexturedQuadGeometry(pos,image->s(),image->t(),image, useTextureRectangle));

                    pos.z() += image->t()*1.5f;
                }
                else
                {
                    std::cout<<"Unable to read file "<<arguments[i]<<std::endl;
                }            
            }
        }

        // set the scene to render
        viewer.setSceneData(geode.get());

        if (viewer.getSceneData()==0)
        {
            arguments.getApplicationUsage()->write(std::cout);
            return 1;
        }
    }
    

    // pass the model to the MovieEventHandler so it can pick out ImageStream's to manipulate.
    MovieEventHandler* meh = new MovieEventHandler();
    meh->set(viewer.getSceneData());
    viewer.addEventHandler(meh);

    // report any errors if they have occured when parsing the program aguments.
    if (arguments.errors())
    {
        arguments.writeErrorMessages(std::cout);
        return 1;
    }



    // create the windows and run the threads.
    return viewer.run();


}
