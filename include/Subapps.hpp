#pragma once

#include <SanctiaEntity.hpp>
#include <EventGraph.hpp>

class SubApps
{
    private : 


    protected : 

        EntityRef uiMenuParentTMP;
        EntityRef uiChoiceParentTMP;

        EntityRef appRoot;

        static inline  std::vector<SubApps*> apps;

        static inline SubApps *activeApp = nullptr;

        std::string name;

        std::vector<EventInput*> inputs;
        
    public : 

        SubApps(const std::string  &name);

        virtual EntityRef UImenu() = 0;

        virtual EntityRef UIcontrols();

        virtual void init() = 0;

        virtual void update() = 0;

        virtual void clean() = 0;

        static void switchTo(SubApps *ptr);
        static void switchTo(const std::string &name);
        static std::string getActiveAppName() {return activeApp->name;}

        static void UpdateApps();

        static void cleanActiveApp();
};


namespace Apps
{
    class MainGameApp : public SubApps
    {
        private : 

            EntityRef gameUI;

        public : 

            MainGameApp();

            virtual EntityRef UImenu() override;

            virtual void init() override;

            virtual void update() override;

            virtual void clean() override;
    };

    class MaterialViewerApp : public SubApps
    {

        private : 

            std::vector<vec3> color;
            std::vector<vec3> SME;
            std::vector<vec4> PSBD;
            std::vector<vec2> BD;
            std::vector<ModelRef> helpers;

            EntityRef menuInfosTab;
            EntityRef titleTab;

            OrbitController orbitController;

            std::string currentPalette;
            std::unordered_map<std::string, EntityRef> palettesList;

            void refreshPalettesList();

        public : 

            MaterialViewerApp();

            void spawnHelper();

            virtual EntityRef UImenu() override;

            virtual EntityRef UIcontrols() override;

            virtual void init() override;

            virtual void update() override;

            virtual void clean() override;
    };

    class EventGraphApp : public SubApps
    {

        private : 
            EventNodePtr a;
            EventNodePtr b;
            EventNodePtr c;
            EventNodePtr d;
            EventNodePtr e;
            EventNodePtr f;
            EventNodePtr and1;
            EventNodePtr and2;
            EventNodePtr or1;
            EventNodePtr not1;

            DragController2D dragController;

            EntityRef graphView;
            EntityRef viewBG;
        public:
            
                EventGraphApp();
    
                virtual EntityRef UImenu() override;
    
                virtual void init() override;
    
                virtual void update() override;
    
                virtual void clean() override;
    };
    
    class SceneMergeApp : public SubApps
    {
        private :

            OrbitController orbitController;

        public : 

            SceneMergeApp();

            virtual EntityRef UImenu() override;

            virtual void init() override;

            virtual void update() override;

            virtual void clean() override;
    };

    class EntityCreator : public SubApps
    {
        private :

            std::string processNewLoadedChild(Entity *c);

            void UpdateCurrentEntityTransform();

            OrbitController orbitController;

            std::unordered_map<std::string, EntityRef> UI_loadableEntity;
            std::unordered_map<std::string, EntityRef> UI_loadableChildren;

            std::unordered_map<std::string, EntityRef> UI_currentEntityComponent;
            std::unordered_map<std::string, EntityRef> UI_currentEntityChildren;

            Entity *controlledEntity = nullptr;
            vec3 controlledEntityEuleur = vec3(0);

            EntityRef gizmo;

            bool autoRefreshFromDisk = false;

            bool gizmoActivated = true;

            struct CurrentEntityInfo
            {
                std::string name;
                EntityRef ref;

            } currentEntity;

        public : 

            EntityCreator();

            virtual EntityRef UImenu() override;

            virtual EntityRef UIcontrols() override;

            virtual void init() override;

            virtual void update() override;

            virtual void clean() override;
    };
}
