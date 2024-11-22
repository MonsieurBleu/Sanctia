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

        virtual void init() = 0;

        virtual void update() = 0;

        virtual void clean() = 0;

        static void switchTo(SubApps *ptr);

        static void UpdateApps();

        static void cleanActiveApp();
};


namespace Apps
{
    class MainGameApp : public SubApps
    {

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

        public : 

            MaterialViewerApp();

            void spawnHelper();

            virtual EntityRef UImenu() override;

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

            OrbitController orbitController;
        public:
            
                EventGraphApp();
    
                virtual EntityRef UImenu() override;
    
                virtual void init() override;
    
                virtual void update() override;
    
                virtual void clean() override;
    };
}